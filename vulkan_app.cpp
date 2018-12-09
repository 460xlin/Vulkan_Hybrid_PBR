#include "vulkan_app.h"
#include "app_util.h"
#include <tiny_obj_loader.h>
#define TINYOBJLOADER_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <stb_image.h>
#include <gli/gli.hpp>
#include <glm/gtc/constants.hpp>

// Timers =================================================
std::chrono::time_point<std::chrono::steady_clock> START_TIME;
std::chrono::time_point<std::chrono::steady_clock> LAST_RECORD_TIME;

void INIT_GLOBAL_TIME() {
    START_TIME = std::chrono::high_resolution_clock::now();
    LAST_RECORD_TIME = std::chrono::high_resolution_clock::now();
}

void SET_GLOBAL_RECORD_TIME() {
    LAST_RECORD_TIME = std::chrono::high_resolution_clock::now();
}

float GET_CLOBAL_TIME_GAP_SINCE_LAST_RECORD() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - LAST_RECORD_TIME).count();
    return time;
}

float GET_CLOBAL_TIME_GAP_SINCE_START() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - START_TIME).count();
    return time;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

// cam & mouse & input =================================================
bool leftMouseDown = false;
bool rightMouseDown = false;
double previousX = 0.0;
double previousY = 0.0;
Camera* firstPersonCam;

void VulkanApp::initCam() {
    firstPersonCam = new Camera(WIDTH, HEIGHT);
}

void VulkanApp::mouseDownCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            leftMouseDown = true;
            glfwGetCursorPos(window, &previousX, &previousY);
        }
        else if (action == GLFW_RELEASE) {
            leftMouseDown = false;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rightMouseDown = true;
            glfwGetCursorPos(window, &previousX, &previousY);
        }
        else if (action == GLFW_RELEASE) {
            rightMouseDown = false;
        }
    }
}

void VulkanApp::mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition) {
    if (leftMouseDown) {
        double rotateSpeed = 0.3;
        float deltaX = static_cast<float>((previousX - xPosition) * rotateSpeed);
        float deltaY = static_cast<float>((previousY - yPosition) * rotateSpeed);

        firstPersonCam->RotateAboutUp(deltaX);
        firstPersonCam->RotateAboutRight(-deltaY);

        previousX = xPosition;
        previousY = yPosition;
    }
}

void VulkanApp::keyDownCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    float timeGap = GET_CLOBAL_TIME_GAP_SINCE_LAST_RECORD();
    float movementSpeed = 0.05f;
    if (key == GLFW_KEY_W && (action == GLFW_KEY_LAST || GLFW_PRESS)) {
        // go forward
        firstPersonCam->TranslateAlongLook(timeGap * movementSpeed);
    }
    else if (key == GLFW_KEY_S && (action == GLFW_KEY_LAST || GLFW_PRESS)) {
        // go back
        firstPersonCam->TranslateAlongLook(-timeGap * movementSpeed);
    }
    else if (key == GLFW_KEY_A && (action == GLFW_KEY_LAST || GLFW_PRESS)) {
        // go left
        firstPersonCam->TranslateAlongRight(-timeGap * movementSpeed);
    }
    else if (key == GLFW_KEY_D && (action == GLFW_KEY_LAST || GLFW_PRESS)) {
        // go right
        firstPersonCam->TranslateAlongRight(timeGap * movementSpeed);
    }
    else if (key == GLFW_KEY_Q && (action == GLFW_KEY_LAST || GLFW_PRESS)) {
        // go up
        firstPersonCam->TranslateAlongUp(+timeGap * movementSpeed);
    }
    else if (key == GLFW_KEY_E && (action == GLFW_KEY_LAST || GLFW_PRESS)) {
        // go down
        firstPersonCam->TranslateAlongUp(timeGap * movementSpeed);
    }
}



// Init non-vulkan stuff =================================================
void VulkanApp::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window_ = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetMouseButtonCallback(window_, mouseDownCallback);
    glfwSetCursorPosCallback(window_, mouseMoveCallback);
    glfwSetKeyCallback(window_, keyDownCallback);
}

VulkanApp::VulkanApp() {}

void VulkanApp::run() {
    initWindow();
    initCam();
    INIT_GLOBAL_TIME();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void VulkanApp::initVulkan() {
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createSwapChainImageViews();
    initSemAndSubmitInfo();

    createPipelineCache();
    createCommandPool();
    createDescriptorPool();
    createDepthResources();
    setupVertexDescriptions();

    // begin offscreen ==========================================
    // scene objects need offscreen's ubo, so has to be before object

	//prepareSkybox();
 //   prepareSceneObjectsData();

	//// in this prepare offscreen function,
	//// we prepare the renderpass and framebuffer 
	//// which will be used to create skybox pipeline
 //   prepareOffscreen();

	//// because create pipeline need renderpass which now is offscreen.renderpass
	//createSkyboxPipeline();

 //   prepareSceneObjectsDescriptor();
 //   prepareOffscreenCommandBuffer();
 //   prepareDeferred();


	// ray tracing pipeline
	rt_createSema();
	rt_createUniformBuffers();
	rt_prepareStorageBuffers();
	rt_prepareTextureTarget(rt_result, VK_FORMAT_R8G8B8A8_UNORM);
	rt_graphics_setupDescriptorSetLayout();
	rt_graphics_setupDescriptorSet();
	rt_createPipelineLayout();
	rt_createRenderpass();
	rt_creatFramebuffer();
	rt_createPipeline();
	rt_prepareCompute();
	rt_createComputeCommandBuffer();
	rt_createRaytraceDisplayCommandBuffer();
}

void VulkanApp::mainLoop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        rt_draw();
		rt_updateUniformBuffer();

        //draw_new();
        //updateUniformBuffers();
    }

    vkDeviceWaitIdle(device_);
}

void VulkanApp::cleanup() {


    for (auto shaderModule : shaderModules) {
        vkDestroyShaderModule(device_, shaderModule, nullptr);
    }

    vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);

    vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);

    vkDestroyBuffer(device_, quadIndexBuffer, nullptr);
    vkFreeMemory(device_, quadIndexBufferMemory, nullptr);

    vkDestroyBuffer(device_, quadVertexBuffer, nullptr);
    vkFreeMemory(device_, quadVertexBufferMemory, nullptr);

    vkDestroySemaphore(device_, offscreen_semaphore_, nullptr);


    vkDestroyPipelineCache(device_, pipelineCache, nullptr);

    vkDestroyCommandPool(device_, command_pool_, nullptr);

    vkDestroyDevice(device_, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance_, callback_, nullptr);
    }

    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);

    glfwDestroyWindow(window_);

    glfwTerminate();

    delete firstPersonCam;
}

void VulkanApp::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void VulkanApp::setupDebugCallback() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &callback_) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug callback!");
    }
}

void VulkanApp::createSurface() {
    if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void VulkanApp::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physical_device_ = device;
            break;
        }
    }

    if (physical_device_ == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VulkanApp::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physical_device_);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    // for skybox
    deviceFeatures.textureCompressionBC = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &queue_);
    // vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    std::cout << "queue g: " << indices.graphicsFamily.value() << std::endl;
    std::cout << "queue p: " << indices.presentFamily.value() << std::endl;
    std::cout << "queue c: " << indices.computeFamily.value() << std::endl;
    std::cout << "graphicsQueue: " << queue_ << std::endl;
    // std::cout << "presentQueue: " << presentQueue << std::endl;

}

void VulkanApp::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physical_device_);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physical_device_);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    swapchain_images_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchain_images_.data());

    swapchain_imageformat_ = surfaceFormat.format;
    swapchain_extent_ = extent;
}

void VulkanApp::createSwapChainImageViews() {
    swapchain_imageviews_.resize(swapchain_images_.size());

    for (uint32_t i = 0; i < swapchain_images_.size(); i++) {
        swapchain_imageviews_[i] = createImageView(swapchain_images_[i], swapchain_imageformat_, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

VkPipelineShaderStageCreateInfo VulkanApp::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{

    auto shaderCode = readFile(fileName);

    VkShaderModule shaderModule = createShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
    shaderStage.module = shaderModule;
    shaderStage.pName = "main";

    assert(shaderStage.module != VK_NULL_HANDLE);

    shaderModules.push_back(shaderStage.module);
    

    return shaderStage;

}

// after deferred render pass
void VulkanApp::createSwapChainFramebuffers() {
    swapchain_framebuffers_.resize(swapchain_imageviews_.size());

    for (size_t i = 0; i < swapchain_imageviews_.size(); i++) {

        std::vector<VkImageView> attachments = {
            swapchain_imageviews_[i],
            depth_attachment_.imageView
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        // IMPT
        framebufferInfo.renderPass = deferred_.renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchain_extent_.width;
        framebufferInfo.height = swapchain_extent_.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapchain_framebuffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void VulkanApp::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physical_device_);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &command_pool_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

void VulkanApp::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(swapchain_extent_.width, swapchain_extent_.height, depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depth_attachment_.image, depth_attachment_.deviceMemory);

    depth_attachment_.imageView = createImageView(depth_attachment_.image, depthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    transitionImageLayout(depth_attachment_.image, depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void VulkanApp::createPipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    if (vkCreatePipelineCache(device_, &pipelineCacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipelineCache");
    }
}

VkFormat VulkanApp::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device_, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanApp::findDepthFormat() {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool VulkanApp::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkImageView VulkanApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device_, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void VulkanApp::createImage(uint32_t width, uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory) {

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device_, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device_, image, imageMemory, 0);
}

void VulkanApp::transitionImageLayout(VkImage image, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        // here
        barrier.srcAccessMask = 0;
        // TODO: maybe wrong
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void VulkanApp::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void VulkanApp::loadSingleSceneObjectMesh(AppSceneObject& object_struct) {
    struct Vertex {
        float pos[3];
        float uv[2];
        float col[3];
        float normal[3];
        float tangent[3];
    };

    std::string file_path = object_struct.meshPath;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;



    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    int colorRounding = 1333;
    for (const auto& shape : shapes) {
        // +3 for per triangle
        for (int i = 0; i < shape.mesh.indices.size(); i += 3) {
            colorRounding += 1333333;
            Vertex verts[3];
            const tinyobj::index_t idx[] = {
                shape.mesh.indices[i],
                shape.mesh.indices[i + 1],
                shape.mesh.indices[i + 2] };

            float colorVul[3];
            colorVul[0] = (float)(colorRounding % 255) / (float)255;
            colorRounding += 345256;
            colorVul[1] = 0.f;
            colorVul[2] = (float)(colorRounding % 255) / (float)255;

            for (int j = 0; j < 3; ++j) {
                const auto& index = idx[j];
                auto& vert = verts[j];
                vert.pos[0] = attrib.vertices[3 * index.vertex_index + 2];
                vert.pos[1] = attrib.vertices[3 * index.vertex_index + 1];
                vert.pos[2] = attrib.vertices[3 * index.vertex_index + 0];

                vert.uv[0] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 0];
                vert.uv[1] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];

                vert.col[0] = colorVul[0];
                vert.col[1] = colorVul[1];
                vert.col[2] = colorVul[2];

                vert.normal[0] = attrib.normals[3 * index.normal_index + 2];
                vert.normal[1] = attrib.normals[3 * index.normal_index + 1];
                vert.normal[2] = attrib.normals[3 * index.normal_index + 0];
            }

            // clac tangent
            // Edges of the triangle : position delta
            glm::vec3 deltaPos1 = glm::vec3(
                verts[1].pos[0] - verts[0].pos[0],
                verts[1].pos[1] - verts[0].pos[1],
                verts[1].pos[2] - verts[0].pos[2]);
            glm::vec3 deltaPos2 = glm::vec3(
                verts[2].pos[0] - verts[0].pos[0],
                verts[2].pos[1] - verts[0].pos[1],
                verts[2].pos[2] - verts[0].pos[2]);

            // UV delta
            glm::vec2 deltaUV1 = glm::vec2(
                verts[1].uv[0] - verts[0].uv[0],
                verts[1].uv[1] - verts[0].uv[1]
            );
            glm::vec2 deltaUV2 = glm::vec2(
                verts[2].uv[0] - verts[0].uv[0],
                verts[2].uv[1] - verts[0].uv[1]
            );

            float r = 1.0f / (deltaUV1.x*deltaUV2.y - deltaUV1.y*deltaUV2.x);
            glm::vec3 tangent = r
                * (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y);

            // write tangent
            for (int j = 0; j < 3; ++j) {
                verts[j].tangent[0] = tangent.x;
                verts[j].tangent[1] = tangent.y;
                verts[j].tangent[2] = tangent.z;
            }

            // push_back
            for (int j = 0; j < 3; ++j) {
                vertices.push_back(verts[j]);
                indices.push_back(indices.size());
            }
        }
    }
    //for (const auto& shape : shapes) {
    //    for (const auto& index : shape.mesh.indices) {
    //        colorRounding += 37;
    //        Vertex vertex = {};

    //        vertex.pos[0] = attrib.vertices[3 * index.vertex_index + 2];
    //        vertex.pos[1] = attrib.vertices[3 * index.vertex_index + 1];
    //        vertex.pos[2] = attrib.vertices[3 * index.vertex_index + 0];

    //        // IMPT
    //        /*vertex.uv[0] = attrib.texcoords[2 * index.texcoord_index + 0];
    //        vertex.uv[1] = attrib.texcoords[2 * index.texcoord_index + 1];*/

    //        vertex.uv[0] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 0];
    //        vertex.uv[1] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];

    //        vertex.col[0] = (float)(255 % colorRounding) / (float)255;
    //        vertex.col[1] = 0.f;
    //        vertex.col[2] = (float)(255 / colorRounding) / (float)255;

    //        vertex.normal[0] = attrib.normals[3 * index.normal_index + 2];
    //        vertex.normal[1] = attrib.normals[3 * index.normal_index + 1];
    //        vertex.normal[2] = attrib.normals[3 * index.normal_index + 0];

    //        vertex.tangent[0] = 0.3f;
    //        vertex.tangent[1] = 0.6f;
    //        vertex.tangent[2] = 0.3f;

    //        vertices.push_back(vertex);
    //        indices.push_back(indices.size());
    //    }
    //}

    object_struct.vertexCount = static_cast<uint32_t>(vertices.size());
    object_struct.indexCount = static_cast<uint32_t>(indices.size());

    // create vertex buffer for arbitary  model
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();

    VkBuffer vertexStagingBuffer;
    VkDeviceMemory vertexStagingBufferMemory;
    createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexStagingBuffer, vertexStagingBufferMemory);

    void* vertexData;
    vkMapMemory(device_, vertexStagingBufferMemory, 0, vertexBufferSize, 0,
        &vertexData);
    memcpy(vertexData, vertices.data(), (size_t)vertexBufferSize);
    vkUnmapMemory(device_, vertexStagingBufferMemory);

    createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        object_struct.vertexBuffer.buffer,
        object_struct.vertexBuffer.deviceMemory
    );

    copyBuffer(vertexStagingBuffer, object_struct.vertexBuffer.buffer,
        vertexBufferSize);

    vkDestroyBuffer(device_, vertexStagingBuffer, nullptr);
    vkFreeMemory(device_, vertexStagingBufferMemory, nullptr);

    // create index buffer for arbitary  model
    VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer indexStagingBuffer;
    VkDeviceMemory indexStagingBufferMemory;
    createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexStagingBuffer, indexStagingBufferMemory);

    void* indexData;
    vkMapMemory(device_, indexStagingBufferMemory, 0, indexBufferSize, 0,
        &indexData);
    memcpy(indexData, indices.data(), (size_t)indexBufferSize);
    vkUnmapMemory(device_, indexStagingBufferMemory);

    createBuffer(indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        object_struct.indexBuffer.buffer, object_struct.indexBuffer.deviceMemory);

    copyBuffer(indexStagingBuffer, object_struct.indexBuffer.buffer,
        indexBufferSize);

    vkDestroyBuffer(device_, indexStagingBuffer, nullptr);
    vkFreeMemory(device_, indexStagingBufferMemory, nullptr);
}



void VulkanApp::createDescriptorPool() {
    // todo check if all pipelins share the same decriptor pool
    std::array<VkDescriptorPoolSize, 4> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 20;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 20;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[2].descriptorCount = 20;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[3].descriptorCount = 20;


    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    //poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
    poolInfo.maxSets = 50;

    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void VulkanApp::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory) 
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}

VkCommandBuffer VulkanApp::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool_;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanApp::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue_);

    vkFreeCommandBuffers(device_, command_pool_, 1, &commandBuffer);
}

void VulkanApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


VkShaderModule VulkanApp::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

VkSurfaceFormatKHR VulkanApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
        else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

VkExtent2D VulkanApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

SwapChainSupportDetails VulkanApp::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool VulkanApp::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate  && supportedFeatures.samplerAnisotropy;
}

bool VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanApp::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (const auto& queueFamily : queueFamilies) {
        std::cout << "queueFlags: " << queueFamily.queueFlags << std::endl;
        std::cout << "queueCount: " << queueFamily.queueCount << std::endl;
        std::cout << "timestampValidBits: " << queueFamily.timestampValidBits << std::endl;
        // std::cout << "minImageTransferGranularity: " << queueFamily.minImageTransferGranularity << std::endl;
    }

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
        }


        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.computeFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

std::vector<const char*> VulkanApp::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool VulkanApp::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<char> VulkanApp::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}


void VulkanApp::createQuadVertexBuffer()
{
    struct Vertex {
        float pos[3];
        float uv[2];
        float col[3];
        float normal[3];
        float tangent[3];
    };

    std::vector<Vertex> tempVertexBuffer;
  
    float x = 0.0f;
    float y = 0.0f;
    for (uint32_t i = 0; i < 1; i++)
    {
        // Last component of normal is used for debug display sampler index
        tempVertexBuffer.push_back({ { x + 1.0f, y + 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, (float)i } });
        tempVertexBuffer.push_back({ { x,      y + 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, (float)i } });
        tempVertexBuffer.push_back({ { x,      y,      0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, (float)i } });
        tempVertexBuffer.push_back({ { x + 1.0f, y,      0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, (float)i } });
        x += 1.0f;
        if (x > 1.0f)
        {
            x = 0.0f;
            y += 1.0f;
        }
    }

    VkDeviceSize bufferSize = sizeof(tempVertexBuffer[0]) * tempVertexBuffer.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, tempVertexBuffer.data(), (size_t)bufferSize);
    vkUnmapMemory(device_, stagingBufferMemory);

    createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, quadVertexBuffer, quadVertexBufferMemory);
    copyBuffer(stagingBuffer, quadVertexBuffer, bufferSize);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

void VulkanApp::createQuadIndexBuffer() {

    std::vector<uint32_t> tempIndexBuffer = { 0,1,2, 2,3,0 };

    for (int i = 0; i < tempIndexBuffer.size(); ++i) {
        std::cout << tempIndexBuffer[i] << " ";
    }
    std::cout << std::endl;
    VkDeviceSize bufferSize = sizeof(tempIndexBuffer[0]) * tempIndexBuffer.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, tempIndexBuffer.data(), (size_t)bufferSize);
    vkUnmapMemory(device_, stagingBufferMemory);

    createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, quadIndexBuffer, quadIndexBufferMemory);

    copyBuffer(stagingBuffer, quadIndexBuffer, bufferSize);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

void VulkanApp::prepareQuadVertexAndIndexBuffer()
{
    createQuadVertexBuffer();
    createQuadIndexBuffer();
}

void VulkanApp::setupVertexDescriptions() {
    vertex_input_info.bindingDescriptions.resize(1);
    VkVertexInputBindingDescription vInputBindDescription{};
    vInputBindDescription.binding = 0;
    vInputBindDescription.stride = 56;
    vInputBindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input_info.bindingDescriptions[0] = vInputBindDescription;
    vertex_input_info.attributeDescriptions.resize(5);

    // Location 0: pos
    VkVertexInputAttributeDescription posAttrDesc{};
    posAttrDesc.location = 0;
    posAttrDesc.binding = 0;
    posAttrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    posAttrDesc.offset = 0;

    // Location 1: uv
    VkVertexInputAttributeDescription uvAttrDesc{};
    uvAttrDesc.location = 1;
    uvAttrDesc.binding = 0;
    uvAttrDesc.format = VK_FORMAT_R32G32_SFLOAT;
    uvAttrDesc.offset = sizeof(float) * 3;

    // Location 2: color
    VkVertexInputAttributeDescription colorAttrDesc{};
    colorAttrDesc.location = 2;
    colorAttrDesc.binding = 0;
    colorAttrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttrDesc.offset = sizeof(float) * 5;

    // Location 3: normal
    VkVertexInputAttributeDescription normalAttrDesc{};
    normalAttrDesc.location = 3;
    normalAttrDesc.binding = 0;
    normalAttrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttrDesc.offset = sizeof(float) * 8;

    // Location 4: tangent
    VkVertexInputAttributeDescription tanAttrDesc{};
    tanAttrDesc.location = 4;
    tanAttrDesc.binding = 0;
    tanAttrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    tanAttrDesc.offset = sizeof(float) * 11;

    vertex_input_info.attributeDescriptions[0] = posAttrDesc;
    vertex_input_info.attributeDescriptions[1] = uvAttrDesc;
    vertex_input_info.attributeDescriptions[2] = colorAttrDesc;
    vertex_input_info.attributeDescriptions[3] = normalAttrDesc;
    vertex_input_info.attributeDescriptions[4] = tanAttrDesc;

    VkPipelineVertexInputStateCreateInfo inputStateInfo{};
    inputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputStateInfo.vertexBindingDescriptionCount =
        static_cast<uint32_t>(vertex_input_info.bindingDescriptions.size());
    inputStateInfo.pVertexBindingDescriptions =
        vertex_input_info.bindingDescriptions.data();
    inputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertex_input_info.attributeDescriptions.size());
    inputStateInfo.pVertexAttributeDescriptions =
        vertex_input_info.attributeDescriptions.data();

    vertex_input_info.inputState = inputStateInfo;
}


void VulkanApp::initSemAndSubmitInfo()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &semaphores_.presentComplete) != VK_SUCCESS)
    {
        throw std::runtime_error("failed tp create presentComplete semaphore");
    }
    if (vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &semaphores_.renderComplete) != VK_SUCCESS)
    {
        throw std::runtime_error("failed tp create presentComplete semaphore");
    }

    mySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
}

VkResult VulkanApp::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;
    if (waitSemaphore != VK_NULL_HANDLE)
    {
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.waitSemaphoreCount = 1;
    }
    VkResult res = vkQueuePresentKHR(queue, &presentInfo);
    return res;
}

// ray tracing =================================================
void VulkanApp::rt_createSema()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &rt_sema) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreenSemaphore");
	}
}

void VulkanApp::rt_createUniformBuffers() {
	VkDeviceSize rtBufferSize = sizeof(RTUniformBufferObject);
	createBuffer(rtBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		uniformBuffers.rt_compute.buffer, uniformBuffers.rt_compute.deviceMem
	);
}

void VulkanApp::rt_prepareStorageBuffers() {
   
    // Spheres
    std::vector<Sphere> spheres;
    spheres.push_back(newSphere(glm::vec3(1.75f, -0.5f, 0.0f), 1.0f, glm::vec3(0.0f, 1.0f, 0.0f), 32.0f));
    spheres.push_back(newSphere(glm::vec3(0.0f, 1.0f, -0.5f), 1.0f, glm::vec3(0.65f, 0.77f, 0.97f), 32.0f));
    spheres.push_back(newSphere(glm::vec3(-1.75f, -0.75f, -0.5f), 1.25f, glm::vec3(0.9f, 0.76f, 0.46f), 32.0f));
    VkDeviceSize sphereStorageBufferSize = spheres.size() * sizeof(Sphere);

    VkBuffer sphereStagingBuffer;
    VkDeviceMemory sphereStagingBufferMemory;
    createBuffer(sphereStorageBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sphereStagingBuffer, sphereStagingBufferMemory);

    void* sphere_data;
    vkMapMemory(device_, sphereStagingBufferMemory, 0, sphereStorageBufferSize, 0, &sphere_data);
    memcpy(sphere_data, spheres.data(), (size_t)sphereStorageBufferSize);
    vkUnmapMemory(device_, sphereStagingBufferMemory);

    createBuffer(sphereStorageBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, compute.mySphereBuffer.buffer, compute.mySphereBuffer.deviceMem);
    copyBuffer(sphereStagingBuffer, compute.mySphereBuffer.buffer, sphereStorageBufferSize);

    vkDestroyBuffer(device_, sphereStagingBuffer, nullptr);
    vkFreeMemory(device_, sphereStagingBufferMemory, nullptr);
    // Copy to staging buffer

    // Planes
    std::vector<Plane> planes;
    const float roomDim = 4.0f;
    planes.push_back(newPlane(glm::vec3(0.0f, 1.0f, 0.0f), roomDim, glm::vec3(1.0f), 32.0f));
    planes.push_back(newPlane(glm::vec3(0.0f, -1.0f, 0.0f), roomDim, glm::vec3(1.0f), 32.0f));
    planes.push_back(newPlane(glm::vec3(0.0f, 0.0f, 1.0f), roomDim, glm::vec3(1.0f), 32.0f));
    planes.push_back(newPlane(glm::vec3(0.0f, 0.0f, -1.0f), roomDim, glm::vec3(0.0f), 32.0f));
    planes.push_back(newPlane(glm::vec3(-1.0f, 0.0f, 0.0f), roomDim, glm::vec3(1.0f, 0.0f, 0.0f), 32.0f));
    planes.push_back(newPlane(glm::vec3(1.0f, 0.0f, 0.0f), roomDim, glm::vec3(0.0f, 1.0f, 0.0f), 32.0f));


    VkDeviceSize planeStorageBufferSize = planes.size() * sizeof(Plane);

    VkBuffer planeStagingBuffer;
    VkDeviceMemory planeStagingBufferMemory;
    createBuffer(planeStorageBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        planeStagingBuffer, planeStagingBufferMemory);

    void* plane_data;
    vkMapMemory(device_, planeStagingBufferMemory, 0, planeStorageBufferSize, 0, &plane_data);
    memcpy(plane_data, planes.data(), (size_t)planeStorageBufferSize);
    vkUnmapMemory(device_, planeStagingBufferMemory);

    createBuffer(planeStorageBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, compute.myPlaneBuffer.buffer, compute.myPlaneBuffer.deviceMem);
    copyBuffer(planeStagingBuffer, compute.myPlaneBuffer.buffer, planeStorageBufferSize);

    vkDestroyBuffer(device_, planeStagingBuffer, nullptr);
    vkFreeMemory(device_, planeStagingBufferMemory, nullptr);
}
void VulkanApp::rt_prepareTextureTarget(MyTexture& tex, VkFormat format, uint32_t width, uint32_t height)
{
	// Get device properties for the requested texture format
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physical_device_, format, &formatProperties);
	// Check if requested image format supports image storage operations
	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);

	// Prepare blit target texture
	createImage(width, height, format,
		VK_IMAGE_TILING_OPTIMAL, // tiling
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, // usage
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //properties: VkMemoryPropertyFlags
		tex.textureImage, //image
		tex.textureImageMemory); //imagemem

	transitionImageLayout(tex.textureImage, format,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL);

	// todo: no need to transfer image from global to local mem
	// here  
	VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 1.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = 0.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;


	if (vkCreateSampler(device_, &sampler, nullptr, &tex.textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create rt_result.textureSampler!");
	}

	tex.textureImageView = createImageView(tex.textureImage, format,
		VK_IMAGE_ASPECT_COLOR_BIT);

	// todo: to set descriptor set info for create descriptor set
	//tex->descriptor.imageLayout = tex->imageLayout;
	//tex->descriptor.imageView = tex->view;
	//tex->descriptor.sampler = tex->sampler;
	//tex->device = vulkanDevice;
}





void VulkanApp::rt_graphics_setupDescriptorSetLayout() {
    // Binding 0 : Fragment shader image sampler

    VkDescriptorSetLayoutBinding setLayoutBinding{};
    setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    setLayoutBinding.binding = 0;
    setLayoutBinding.descriptorCount = 1;

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {setLayoutBinding};

    VkDescriptorSetLayoutCreateInfo descriptorLayout{};
    descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.pBindings = setLayoutBindings.data();
    descriptorLayout.bindingCount = setLayoutBindings.size();

    // here
    if (vkCreateDescriptorSetLayout(device_, &descriptorLayout, nullptr, &graphics.rt_descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptorlayout for rt!");
    }


   
}

void VulkanApp::rt_graphics_setupDescriptorSet()
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptor_pool_;
	allocInfo.pSetLayouts = &graphics.rt_descriptorSetLayout;
	// allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;
	if (vkAllocateDescriptorSets(device_, &allocInfo, &graphics.rt_descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create rt_descriptorSet for rt!");
	}

	// todo: to set descriptor set info for create descriptor set

	VkDescriptorImageInfo rt_imageInfo{};
	rt_imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	rt_imageInfo.imageView = rt_result.textureImageView;
	rt_imageInfo.sampler = rt_result.textureSampler;

	// Binding 0 : Fragment shader texture sampler
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = graphics.rt_descriptorSet;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.pImageInfo = &rt_imageInfo;
	writeDescriptorSet.descriptorCount = 1;

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = { writeDescriptorSet };

	int a = 0;
	++a;

	vkUpdateDescriptorSets(device_, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

}

void VulkanApp::rt_createPipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &graphics.rt_descriptorSetLayout;

	if (vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, nullptr, &graphics.rt_raytracePipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create rt_raytracePipelineLayout for rt!");
	}
}

void VulkanApp::rt_createRenderpass()
{
	// still black box here
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchain_imageformat_;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	// TODO do what?
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;
	subpass.pResolveAttachments = nullptr;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
		| VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
		| VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {
		colorAttachment, depthAttachment
	};

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device_, &renderPassInfo, nullptr,
		&graphics.rt_rayTraceRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics.rt_rayTraceRenderPass!");
	}
}

void VulkanApp::rt_creatFramebuffer()
{
	swapchain_framebuffers_.resize(swapchain_imageviews_.size());

	for (size_t i = 0; i < swapchain_imageviews_.size(); i++) {

		std::vector<VkImageView> attachments = {
			swapchain_imageviews_[i],
			depth_attachment_.imageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// IMPT
		framebufferInfo.renderPass = graphics.rt_rayTraceRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchain_extent_.width;
		framebufferInfo.height = swapchain_extent_.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapchain_framebuffers_[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VulkanApp::rt_createPipeline() 
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
		apputil::createInputAssemblyStateCreateInfo(0,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterizationState{};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.colorWriteMask = 0xf;
	blendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &blendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_FALSE;
	depthStencilState.depthWriteEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.front = depthStencilState.back;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.flags = 0;


	VkPipelineMultisampleStateCreateInfo multisampleState{};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.flags = 0;

	std::vector<VkDynamicState> dynamicStateEnables = {
			 VK_DYNAMIC_STATE_VIEWPORT,
			 VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicState.flags = 0;

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	shaderStages[0] = loadShader(
		"../../shaders/texture.vert.spv",
		VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(
		"../../shaders/texture.frag.spv",
		VK_SHADER_STAGE_FRAGMENT_BIT);

	std::array<VkPipelineColorBlendAttachmentState, 1> blendAttachmentStates = {
			blendAttachmentState
	};

	//std::array<VkPipelineColorBlendAttachmentState, 1> blendAttachmentStates = {
	//	blendAttachmentState,
	//};

	colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
	colorBlendState.pAttachments = blendAttachmentStates.data();

	VkPipelineVertexInputStateCreateInfo emptyInputState{};
	emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	emptyInputState.vertexAttributeDescriptionCount = 0;
	emptyInputState.pVertexAttributeDescriptions = nullptr;
	emptyInputState.vertexBindingDescriptionCount = 0;
	emptyInputState.pVertexBindingDescriptions = nullptr;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = graphics.rt_raytracePipelineLayout;
	pipelineCreateInfo.renderPass = graphics.rt_rayTraceRenderPass;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &emptyInputState;

	if (vkCreateGraphicsPipelines(device_, pipelineCache, 1,
		&pipelineCreateInfo, nullptr, &graphics.rt_raytracePipeline)
		!= VK_SUCCESS) {

		throw std::runtime_error("failed to create offscreen_.pipeline");
	}
}




void VulkanApp::rt_prepareCompute() {

    QueueFamilyIndices fi = findQueueFamilies(physical_device_);

    // todo: what is this for?....................
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.pNext = NULL;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.queueFamilyIndex = fi.computeFamily.value();

    vkGetDeviceQueue(device_, fi.computeFamily.value(), 0, &compute.rt_computeQueue);

    // Binding 0: Storage image (raytraced output)
    VkDescriptorSetLayoutBinding rt_storage;
    rt_storage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    rt_storage.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    rt_storage.binding = 0;
    rt_storage.descriptorCount = 1;

    // Binding 1: Uniform buffer block
    VkDescriptorSetLayoutBinding rt_uniform;
    rt_uniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    rt_uniform.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    rt_uniform.binding = 1;
    rt_uniform.descriptorCount = 1;

    // Binding 2: Shader storage buffer for the spheres
    VkDescriptorSetLayoutBinding rt_sphere;
    rt_sphere.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rt_sphere.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    rt_sphere.binding = 2;
    rt_sphere.descriptorCount = 1;

    // Binding 3: Shader storage buffer for the planes
    VkDescriptorSetLayoutBinding rt_plane;
    rt_plane.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rt_plane.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    rt_plane.binding = 3;
    rt_plane.descriptorCount = 1;

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        rt_storage, rt_uniform, rt_sphere, rt_plane
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayout{};
    descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.pBindings = setLayoutBindings.data();
    descriptorLayout.bindingCount = setLayoutBindings.size();

    if (vkCreateDescriptorSetLayout(device_, &descriptorLayout, nullptr, &compute.rt_computeDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create rt_computeDescriptorSetLayout!");
    }

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo{};
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.setLayoutCount = 1;
    pPipelineLayoutCreateInfo.pSetLayouts = &compute.rt_computeDescriptorSetLayout;

    if (vkCreatePipelineLayout(device_, &pPipelineLayoutCreateInfo, nullptr, &compute.rt_computePipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create rt_computePipelineLayout!");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool_;
    allocInfo.pSetLayouts = &compute.rt_computeDescriptorSetLayout;
    allocInfo.descriptorSetCount = 1;

    if (vkAllocateDescriptorSets(device_, &allocInfo, &compute.rt_computeDescriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create rt_computeDescriptorSet!");
    }

    // TODO: update imageinfo and bufferinfo for write descriptor set
    VkDescriptorImageInfo rt_out_storage_imageInfo = {};
    rt_out_storage_imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    rt_out_storage_imageInfo.sampler = rt_result.textureSampler;
    rt_out_storage_imageInfo.imageView = rt_result.textureImageView;

    // to check out whether offset is 0;
    VkDescriptorBufferInfo rt_uniform_bufferInfo{};
    rt_uniform_bufferInfo.buffer = uniformBuffers.rt_compute.buffer;
    rt_uniform_bufferInfo.offset = 0;
    rt_uniform_bufferInfo.range = sizeof(RTUniformBufferObject);

    // update desbuffinfo for computer's union buffer
    uniformBuffers.rt_compute.desBuffInfo = rt_uniform_bufferInfo;


    VkDescriptorBufferInfo rt_storage_sphere{};
    rt_storage_sphere.buffer = compute.mySphereBuffer.buffer;
    rt_storage_sphere.offset = 0;
    // todo 
    rt_storage_sphere.range = VK_WHOLE_SIZE;
    //rt_storage_sphere.range = 3 * sizeof(Sphere);

    VkDescriptorBufferInfo rt_storage_plane{};
    rt_storage_plane.buffer = compute.myPlaneBuffer.buffer;
    rt_storage_plane.offset = 0;
    //rt_storage_plane.range = VK_WHOLE_SIZE;
    rt_storage_plane.range = 6 * sizeof(Plane);


    // Binding 0: Output storage image
    VkWriteDescriptorSet rt_out_storage{};
    rt_out_storage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    rt_out_storage.dstSet = compute.rt_computeDescriptorSet;
    rt_out_storage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    rt_out_storage.dstBinding = 0;
    rt_out_storage.pImageInfo = &rt_out_storage_imageInfo;
    rt_out_storage.descriptorCount = 1;

    // Binding 1: Uniform buffer block
    VkWriteDescriptorSet rt_uni_block{};
    rt_uni_block.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    rt_uni_block.dstSet = compute.rt_computeDescriptorSet;
    rt_uni_block.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    rt_uni_block.dstBinding = 1;
    rt_uni_block.pBufferInfo = &uniformBuffers.rt_compute.desBuffInfo;
    rt_uni_block.descriptorCount = 1;

    // Binding 2: Shader storage buffer for the spheres
    VkWriteDescriptorSet rt_uni_storage_sphere{};
    rt_uni_storage_sphere.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    rt_uni_storage_sphere.dstSet = compute.rt_computeDescriptorSet;
    rt_uni_storage_sphere.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rt_uni_storage_sphere.dstBinding = 2;
    rt_uni_storage_sphere.pBufferInfo = &rt_storage_sphere;
    rt_uni_storage_sphere.descriptorCount = 1;

    // Binding 3: Shader storage buffer for the planes
    VkWriteDescriptorSet rt_uni_storage_plane{};
    rt_uni_storage_plane.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    rt_uni_storage_plane.dstSet = compute.rt_computeDescriptorSet;
    rt_uni_storage_plane.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rt_uni_storage_plane.dstBinding = 3;
    rt_uni_storage_plane.pBufferInfo = &rt_storage_plane;
    rt_uni_storage_plane.descriptorCount = 1;

    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
    {
        rt_out_storage, rt_uni_block, rt_uni_storage_sphere, rt_uni_storage_plane
    };

    vkUpdateDescriptorSets(device_, computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);


    // Create compute shader pipelines
    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = compute.rt_computePipelineLayout;
    computePipelineCreateInfo.flags = 0;
    computePipelineCreateInfo.stage = loadShader("../../shaders/raytracing.comp.spv",
        VK_SHADER_STAGE_COMPUTE_BIT);

    if (vkCreateComputePipelines(device_, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &compute.rt_computePipine) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute.rt_computePipine!");

    }

    //// Separate command pool as queue family for compute may be different than graphics
    //VkCommandPoolCreateInfo cmdPoolInfo = {};
    //cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //cmdPoolInfo.queueFamilyIndex = fi.computeFamily.value();
    //cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //if (vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &compute.rt_) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create commandPool");
    //}

   
}

void VulkanApp::rt_createComputeCommandBuffer() {

	VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = command_pool_;
	cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocateInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, &compute.rt_computeCmdBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate rt_computeCmdBuffer");
	}

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(device_, &fenceCreateInfo, nullptr, &compute.rt_fence)) {
		throw std::runtime_error("failed to create compute.rt_fence!");
	}

	// buildComputeCommandBuffer
	VkCommandBufferBeginInfo cmdBufBeginInfo{};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (vkBeginCommandBuffer(compute.rt_computeCmdBuffer, &cmdBufBeginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin rt_computeCmdBuffer");
	}

	vkCmdBindPipeline(compute.rt_computeCmdBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		compute.rt_computePipine);
	vkCmdBindDescriptorSets(compute.rt_computeCmdBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE, compute.rt_computePipelineLayout,
		0, 1, &compute.rt_computeDescriptorSet, 0, 0);

	// TODO why 16
	vkCmdDispatch(compute.rt_computeCmdBuffer, swapchain_extent_.width / 16, swapchain_extent_.height / 16, 1);

	vkEndCommandBuffer(compute.rt_computeCmdBuffer);
}

void VulkanApp::rt_createRaytraceDisplayCommandBuffer() {
    rt_drawCommandBuffer.resize(swapchain_images_.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = command_pool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(rt_drawCommandBuffer.size());

    if (vkAllocateCommandBuffers(device_, &allocInfo, rt_drawCommandBuffer.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate 3 x rt_drawCommandBuffer buffers!");
    }

    for (int32_t i = 0; i < rt_drawCommandBuffer.size(); ++i)
    {
        VkCommandBufferBeginInfo cmdBufBeginInfo{};
        cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VkClearValue clearValues[2];
        clearValues[0].color = { 0.025f, 0.025f, 0.025f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = graphics.rt_rayTraceRenderPass;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = swapchain_extent_.width;
        renderPassBeginInfo.renderArea.extent.height = swapchain_extent_.height;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        // Set target frame buffer
        renderPassBeginInfo.framebuffer = swapchain_framebuffers_[i];

        if (vkBeginCommandBuffer(rt_drawCommandBuffer[i], &cmdBufBeginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording rt_drawCommandBuffer buffer!");
        }

        // Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.image = rt_result.textureImage;
        imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(
            rt_drawCommandBuffer[i],
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        vkCmdBeginRenderPass(rt_drawCommandBuffer[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = swapchain_extent_.width;
        viewport.height = swapchain_extent_.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(rt_drawCommandBuffer[i], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent.width = swapchain_extent_.width;
        scissor.extent.height = swapchain_extent_.height;
        scissor.offset.x = 0.0f;
        scissor.offset.y = 1.0f;
        vkCmdSetScissor(rt_drawCommandBuffer[i], 0, 1, &scissor);

        // Display ray traced image generated by compute shader as a full screen quad
        // Quad vertices are generated in the vertex shader
        vkCmdBindDescriptorSets(rt_drawCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.rt_raytracePipelineLayout, 0, 1, &graphics.rt_descriptorSet, 0, NULL);
        vkCmdBindPipeline(rt_drawCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.rt_raytracePipeline);
        vkCmdDraw(rt_drawCommandBuffer[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(rt_drawCommandBuffer[i]);

        if (vkEndCommandBuffer(rt_drawCommandBuffer[i]) != VK_SUCCESS) {
            throw std::runtime_error("can't end rt_drawCommandBuffer");
        }
    }
}

void VulkanApp::rt_draw() {
    //initial submit info
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    mySubmitInfo.pWaitDstStageMask = waitStages;
    mySubmitInfo.signalSemaphoreCount = 1;
    mySubmitInfo.waitSemaphoreCount = 1;

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_,
        std::numeric_limits<uint64_t>::max(),
        semaphores_.presentComplete,
        VK_NULL_HANDLE, &imageIndex);

    
    VkSemaphore waitSemaphores[] = { semaphores_.presentComplete };
    // submit compute
    // wait for swap chian presentation to finish
    mySubmitInfo.pWaitSemaphores = waitSemaphores;
    mySubmitInfo.pSignalSemaphores = &rt_sema;
    mySubmitInfo.commandBufferCount = 1;
    mySubmitInfo.pCommandBuffers = &compute.rt_computeCmdBuffer;

    // Command buffer to be sumitted to the queue
    if (vkQueueSubmit(queue_, 1, &mySubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("can't submit compute.rt_computeCmdBuffer in graphicsQueue");
    }

    // wait for offsreen
    VkSemaphore waitSemaphores_2[] = { rt_sema };
    mySubmitInfo.pWaitSemaphores = waitSemaphores_2;
    mySubmitInfo.pSignalSemaphores = &semaphores_.renderComplete;
    mySubmitInfo.pCommandBuffers = &rt_drawCommandBuffer[imageIndex];
    if (vkQueueSubmit(queue_, 1, &mySubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit rt_drawCommandBuffer in graphicsQueue");
    }

    VkResult res = queuePresent(queue_, imageIndex, semaphores_.renderComplete);
    vkQueueWaitIdle(queue_);
}

Sphere VulkanApp::newSphere(glm::vec3 pos, float radius, glm::vec3 diffuse, float specular)
{
	Sphere sphere;
	sphere.id = rt_currentId++;
	sphere.pos = pos;
	sphere.radius = radius;
	sphere.diffuse = diffuse;
	sphere.specular = specular;
	return sphere;
}

Plane VulkanApp::newPlane(glm::vec3 normal, float distance, glm::vec3 diffuse, float specular)
{
	Plane plane;
	plane.id = rt_currentId++;
	plane.normal = normal;
	plane.distance = distance;
	plane.diffuse = diffuse;
	plane.specular = specular;
	return plane;
}

void VulkanApp::rt_updateUniformBuffer() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count() / 10.f;

	rt_ubo.lightPos.x = 0.0f + sin(glm::radians(time * 360.0f)) * cos(glm::radians(time * 360.0f)) * 2.0f;
	rt_ubo.lightPos.y = 0.0f + sin(glm::radians(time * 360.0f)) * 2.0f;
	rt_ubo.lightPos.z = 0.0f + cos(glm::radians(time * 360.0f)) * 2.0f;
	float camDelta = (int)(time / 1000) % 10;
	rt_ubo.camera.pos = glm::vec3(camDelta);

	void* data;
	vkMapMemory(device_, uniformBuffers.rt_compute.deviceMem, 0, sizeof(rt_ubo), 0, &data);
	memcpy(data, &rt_ubo, sizeof(rt_ubo));
	vkUnmapMemory(device_, uniformBuffers.rt_compute.deviceMem);
}




// ray tracing end  =================================================


void VulkanApp::prepareOffscreen() {
    createOffscreenDescriptorSetLayout();
    createOffscreenUniformBuffer();
    createOffscreenPipelineLayout();
    createOffscreenRenderPass();
    createOffscreenFrameBuffer();
    createOffscreenPipeline();
    // createOffscreenCommandBuffer(); need to be after create scene object desc
}

void VulkanApp::prepareOffscreenCommandBuffer() {
    //createOffscreenCommandBuffer();
	createOffscreenForSkyboxAndModel();
}

void VulkanApp::createOffscreenUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(AppOffscreenUniformBufferContent);
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        offscreen_.uniformBufferAndContent.uniformBuffer.buffer,
        offscreen_.uniformBufferAndContent.uniformBuffer.deviceMemory
    );

    VkDescriptorBufferInfo& buffer_info =
        offscreen_.uniformBufferAndContent.uniformBuffer.descriptorBufferInfo;

    buffer_info.buffer =
        offscreen_.uniformBufferAndContent.uniformBuffer.buffer;
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;
}

// IMPT: Add skybox as a samplercube to offscreen
void VulkanApp::createOffscreenDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        // binding 0: uniform buffer
        apputil::createDescriptorSetLayoutBinding(
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT),
        // binding 1: model info (mode matrix/pos)
        apputil::createDescriptorSetLayoutBinding(
            1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT),
        // binding 2: albedo texture
        apputil::createDescriptorSetLayoutBinding(
            2,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 3: normal map
        apputil::createDescriptorSetLayoutBinding(
            3,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 4: mrao texture
        apputil::createDescriptorSetLayoutBinding(
            4,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT),
		//// binding 5: cubemap texture
		//apputil::createDescriptorSetLayoutBinding(
		//	5,
		//	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		//	1,
		//	VK_SHADER_STAGE_FRAGMENT_BIT),
		//// binding 6: uniform buf (cam)
		// apputil::createDescriptorSetLayoutBinding(
		//	6,
		//	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		//	1,
		//	VK_SHADER_STAGE_VERTEX_BIT)

    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr,
        &offscreen_.descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create offscreen_.descriptorSetLayout!");
    }    
}

void VulkanApp::createOffscreenPipelineLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &offscreen_.descriptorSetLayout;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, nullptr,
        &offscreen_.pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed at offscreen_.pipelineLayout creation");
    }
}


void VulkanApp::createOffscreenFrameBuffer() {
    // all images use the same sampler
    VkSamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
    samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 1.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    if (vkCreateSampler(device_, &samplerCreateInfo, nullptr,
        &offscreen_.frameBufferAssets.sampler) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create offscreen_.frameBufferAssets.sampler");
    }

    // for output
    // world space pos -------------------------------------------------
    AppTexture& posRef = offscreen_.frameBufferAssets.position;
    VkFormat positionFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    createImage(swapchain_extent_.width, swapchain_extent_.height,
        positionFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        posRef.image,
        posRef.deviceMemory);

    posRef.imageView = createImageView(posRef.image, positionFormat,
            VK_IMAGE_ASPECT_COLOR_BIT);

    posRef.descriptorImageInfo.sampler = offscreen_.frameBufferAssets.sampler;
    posRef.descriptorImageInfo.imageView = posRef.imageView;
    posRef.descriptorImageInfo.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // world space normal -------------------------------------------------
    AppTexture& normalRef = offscreen_.frameBufferAssets.normal;
    VkFormat normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    createImage(swapchain_extent_.width, swapchain_extent_.height, normalFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        normalRef.image,
        normalRef.deviceMemory);
    normalRef.imageView = createImageView(normalRef.image, normalFormat,
            VK_IMAGE_ASPECT_COLOR_BIT);
    normalRef.descriptorImageInfo.sampler = offscreen_.frameBufferAssets.sampler;
    normalRef.descriptorImageInfo.imageView = normalRef.imageView;
    normalRef.descriptorImageInfo.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // color -------------------------------------------------
    AppTexture& colorRef = offscreen_.frameBufferAssets.color;
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    createImage(swapchain_extent_.width, swapchain_extent_.height, colorFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        colorRef.image,
        colorRef.deviceMemory);

    colorRef.imageView = createImageView(colorRef.image, colorFormat,
            VK_IMAGE_ASPECT_COLOR_BIT);

    colorRef.descriptorImageInfo.sampler = offscreen_.frameBufferAssets.sampler;
    colorRef.descriptorImageInfo.imageView = colorRef.imageView;
    colorRef.descriptorImageInfo.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // mrao -------------------------------------------------
    AppTexture& mraoRef = offscreen_.frameBufferAssets.mrao;
    VkFormat mraoFormat = VK_FORMAT_R8G8B8A8_UNORM;
    createImage(swapchain_extent_.width, swapchain_extent_.height, mraoFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mraoRef.image, mraoRef.deviceMemory);

    mraoRef.imageView = createImageView(mraoRef.image, mraoFormat,
        VK_IMAGE_ASPECT_COLOR_BIT);

    mraoRef.imageView =
        createImageView(mraoRef.image, mraoFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    mraoRef.descriptorImageInfo.sampler = offscreen_.frameBufferAssets.sampler;
    mraoRef.descriptorImageInfo.imageView = mraoRef.imageView;
    mraoRef.descriptorImageInfo.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


    // Depth -------------------------------------------------
    VkFormat depthFormat = findDepthFormat();
    createImage(swapchain_extent_.width, swapchain_extent_.height, depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        offscreen_.frameBufferAssets.depth.image,
        offscreen_.frameBufferAssets.depth.deviceMemory);

    offscreen_.frameBufferAssets.depth.imageView = createImageView(
        offscreen_.frameBufferAssets.depth.image,
        depthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    std::array<VkImageView, 5> attachments;
    attachments[0] = offscreen_.frameBufferAssets.position.imageView;
    attachments[1] = offscreen_.frameBufferAssets.normal.imageView;
    attachments[2] = offscreen_.frameBufferAssets.color.imageView;
    attachments[3] = offscreen_.frameBufferAssets.mrao.imageView;
    attachments[4] = offscreen_.frameBufferAssets.depth.imageView;
    
    VkFramebufferCreateInfo fbufCreateInfo = {};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.pNext = NULL;
    fbufCreateInfo.renderPass = offscreen_.renderPass;
    fbufCreateInfo.pAttachments = attachments.data();
    fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbufCreateInfo.width = swapchain_extent_.width;
    fbufCreateInfo.height = swapchain_extent_.height;
    fbufCreateInfo.layers = 1;

    if (vkCreateFramebuffer(device_, &fbufCreateInfo, nullptr,
        &offscreen_.frameBufferAssets.frameBuffer) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create offscreen_.frameBufferAssets.frameBuffer");
    }
}

void VulkanApp::createOffscreenPipeline() {
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        apputil::createInputAssemblyStateCreateInfo(0,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.flags = 0;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.colorWriteMask = 0xf;
    blendAttachmentState.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.front = depthStencilState.back;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.flags = 0;


    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.flags = 0;

    std::vector<VkDynamicState> dynamicStateEnables = {
             VK_DYNAMIC_STATE_VIEWPORT,
             VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    dynamicState.flags = 0;

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
    shaderStages[0] = loadShader(
        "../../shaders/mrt.vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(
        "../../shaders/mrt.frag.spv",
        VK_SHADER_STAGE_FRAGMENT_BIT);

    std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates = {
            blendAttachmentState,
            blendAttachmentState,
            blendAttachmentState,
            blendAttachmentState
    };

	//std::array<VkPipelineColorBlendAttachmentState, 1> blendAttachmentStates = {
	//	blendAttachmentState,
	//};

    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments = blendAttachmentStates.data();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = offscreen_.pipelineLayout;
    pipelineCreateInfo.renderPass = offscreen_.renderPass;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.pVertexInputState = &vertex_input_info.inputState;

    if (vkCreateGraphicsPipelines(device_, pipelineCache, 1,
        &pipelineCreateInfo, nullptr, &offscreen_.pipeline)
        != VK_SUCCESS) {

        throw std::runtime_error("failed to create offscreen_.pipeline");
    }
}

void VulkanApp::createOffscreenRenderPass() {

    VkFormat positionFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat mraoFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat = findDepthFormat();
    // Set up separate renderpass with references to the color and depth attachments
	uint32_t attachmentCount = 5;
    std::array<VkAttachmentDescription, 5> attachmentDescs = {};

    for (uint32_t i = 0; i < attachmentCount; ++i)
    {
        attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        if (i == 4)
        {
            attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else
        {
            attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    attachmentDescs[0].format = positionFormat;
    attachmentDescs[1].format = normalFormat;
    attachmentDescs[2].format = colorFormat;
    attachmentDescs[3].format = mraoFormat;
    attachmentDescs[4].format = depthFormat;


    std::vector<VkAttachmentReference> colorReferences;
    colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    colorReferences.push_back({ 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 4;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pDepthStencilAttachment = &depthReference;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = attachmentDescs.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &offscreen_.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass for offscreen frame buffer");
    }
}

// scene objects =================================================
void VulkanApp::prepareSceneObjectsData() {
    AppSceneObject rock{};
    rock.meshPath = "../../models/rock.obj";
    rock.albedo.path = "../../textures/rock_low_Base_Color.png";
    rock.normal.path = "../../textures/rock_low_Normal_DirectX.png";
    rock.mrao.path = "../../textures/rock_low_Normal_DirectX.png";
    

    AppSceneObject cube2{};
    cube2.meshPath = "../../models/cube.obj";
    cube2.albedo.path = "../../textures/uv_debug.png";
    cube2.normal.path = "../../textures/normal_map_debug.png";
    cube2.mrao.path = "../../textures/rock_low_Normal_DirectX.png";


    scene_objects_.push_back(cube2);
    scene_objects_.push_back(rock);



    // the following 2 functions must be called before scene object loading
    // scene object descriptor set requires offscreen uniform buffer and
    // descriptor set layout
    // createOffscreenDescriptorSetLayout()
    // createOffscreenUniformBuffer()

    for (auto& scene_object : scene_objects_) {
        loadSingleSceneObjectMesh(scene_object);
        // texture
        // loadSceneObjectTexture(scene_object);
        loadSingleSceneObjectTexture(scene_object.albedo);
        loadSingleSceneObjectTexture(scene_object.normal);
        loadSingleSceneObjectTexture(scene_object.mrao);
        // uniform buffer
        createModelMatrixUniformBuffer(scene_object);
    }
}

void VulkanApp::prepareSceneObjectsDescriptor() {
    for (auto& scene_object : scene_objects_) {
        // allocate descriptor
        createSceneObjectDescriptorSet(scene_object);
    }
	//auto& skybox_scene_object = skybox_.skyBoxCube.mesh;
	//createSceneObjectDescriptorSet(skybox_scene_object);
}

void VulkanApp::loadSingleSceneObjectTexture(AppTextureInfo& texture_info) {
    // load file
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texture_info.path.c_str(),
        &texWidth, &texHeight, &texChannels,
        STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load abledo texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;

    vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device_, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture_info.texture.image,
        texture_info.texture.deviceMemory);

    texture_info.texture.imageView = createImageView(
        texture_info.texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_ASPECT_COLOR_BIT);

    transitionImageLayout(texture_info.texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, texture_info.texture.image,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight));
    transitionImageLayout(texture_info.texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);

    // create sampler
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device_, &samplerInfo, nullptr,
        &texture_info.texture.sampler) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create texture_info.texture.sampler!");
    }

    // fill image info
    VkDescriptorImageInfo& image_info =
        texture_info.texture.descriptorImageInfo;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.sampler = texture_info.texture.sampler;
    image_info.imageView = texture_info.texture.imageView;
}

void VulkanApp::createSceneObjectDescriptorSet(AppSceneObject& scene_object) {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &offscreen_.descriptorSetLayout;


    if (vkAllocateDescriptorSets(device_, &allocInfo,
        &scene_object.descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "failed to allocate scene_object.descriptorSet");
    }

    std::vector<VkWriteDescriptorSet> write_sets = {
        // binding 0: offscreen uniformBuffer
        apputil::createBufferWriteDescriptorSet(
            scene_object.descriptorSet,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            0,
            &offscreen_.uniformBufferAndContent.uniformBuffer
                .descriptorBufferInfo,
            1),
        // binding 1: self uniformBuffer
        apputil::createBufferWriteDescriptorSet(
            scene_object.descriptorSet,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            &scene_object.uniformBufferAndContent.uniformBuffer
                .descriptorBufferInfo,
            1),
        // binding 2: albedo texture
        apputil::createImageWriteDescriptorSet(
            scene_object.descriptorSet,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            2,
            &scene_object.albedo.texture.descriptorImageInfo,
            1),
        // binding 3: normal map
        apputil::createImageWriteDescriptorSet(
            scene_object.descriptorSet,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            3,
            &scene_object.normal.texture.descriptorImageInfo,
            1),
        // binding 4: mrao texture
        apputil::createImageWriteDescriptorSet(
            scene_object.descriptorSet,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            4,
            &scene_object.mrao.texture.descriptorImageInfo,
            1),
		//// binding 5: cube map tex
		//apputil::createImageWriteDescriptorSet(
		//	scene_object.descriptorSet,
		//	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		//	5,
		//	&skybox_.skyBoxCube.cubemap.textureInfo.texture.descriptorImageInfo,
		//	1),
		//// binding 6: uniform buf (cam)
		//apputil::createBufferWriteDescriptorSet(
		//	scene_object.descriptorSet,
		//	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		//	6,
		//	&skybox_.uniformBufferAndContent.uniformBuffer
		//	.descriptorBufferInfo,
		//	1),
    };

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(write_sets.size()),
        write_sets.data(), 0, NULL);
}

void VulkanApp::createModelMatrixUniformBuffer(AppSceneObject& scene_object) {
    
    VkDeviceSize buffSize = sizeof(AppSceneObjectUniformBufferConent);
    createBuffer(buffSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        scene_object.uniformBufferAndContent.uniformBuffer.buffer,
        scene_object.uniformBufferAndContent.uniformBuffer.deviceMemory
    );

    VkDescriptorBufferInfo& buffer_info =
        scene_object.uniformBufferAndContent.uniformBuffer.descriptorBufferInfo;
    
    buffer_info.buffer =
        scene_object.uniformBufferAndContent.uniformBuffer.buffer;
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;

    scene_object.uniformBufferAndContent.content.modelMatrix = glm::mat4(1.f);
}   

// cubemap =================================================
void VulkanApp::prepareSkybox() {
    prepareSkyboxTexture();
    loadSkyboxMesh();
    createSkyboxUniformBuffer();
    createSkyboxDescriptorSetLayout();
    createSkyboxDescriptorSet();
    createSkyboxPipelineLayout();
}


void VulkanApp::prepareSkyboxTexture()
{
    getEnabledFeatures();
    //VkFormat format{};
    VkFormat format;

    std::string filename;
    if (device_features_.textureCompressionBC) {
        filename = "../../textures/cubemap_yokohama_bc3_unorm.ktx";
        format = VK_FORMAT_BC2_UNORM_BLOCK;
    }
    else if (device_features_.textureCompressionASTC_LDR) {
        filename = "../../textures/cubemap_yokohama_astc_8x8_unorm.ktx";
        format = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
    }
    else if (device_features_.textureCompressionETC2) {
        filename = "../../textures/cubemap_yokohama_etc2_unorm.ktx";
        format = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    }
    else {
        throw std::runtime_error("failed to find suitable format for cubemap");
    }
    
    auto& cubemap = skybox_.skyBoxCube.cubemap;

    cubemap.textureInfo.path = filename;

    gli::texture_cube texCube(gli::load(filename.c_str()));
    assert(!texCube.empty());
    
    cubemap.width = texCube.extent().x;
    cubemap.height = texCube.extent().y;
    cubemap.mipLevels = texCube.levels();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    // =======================================================================

    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = texCube.size();
    // This buffer is used as a transfer source for the buffer copy
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufferCreateInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create stagingBuffer");
    }

    // Get memory requirements for the staging buffer (alignment, memory type bits)
    vkGetBufferMemoryRequirements(device_, stagingBuffer, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    // Get memory type index for a host visible buffer
    memAllocInfo.memoryTypeIndex = skybox_getMemoryType(
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    if (vkAllocateMemory(device_, &memAllocInfo, nullptr, &stagingMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate mem for stagingBuffer");

    }
    if (vkBindBufferMemory(device_, stagingBuffer, stagingMemory, 0) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to bind mem for stagingBuffer");

    }

    // Copy texture data into staging buffer
    uint8_t *data;
    if (vkMapMemory(device_, stagingMemory, 0, memReqs.size, 0, (void **)&data) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to map mem for stagingBuffer");

    }
    memcpy(data, texCube.data(), texCube.size());
    vkUnmapMemory(device_, stagingMemory);

    // Create optimal tiled target image
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = cubemap.mipLevels;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { cubemap.width,
        cubemap.height, 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    // Cube faces count as array layers in Vulkan
    imageCreateInfo.arrayLayers = 6;
    // This flag is required for cube map images
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (vkCreateImage(device_, &imageCreateInfo, nullptr,
        &cubemap.textureInfo.texture.image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create cubemap image");

    }// image create done
    // image creation done
    

    // =========================================================================
    vkGetImageMemoryRequirements(device_, cubemap.textureInfo.texture.image, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = findMemoryType(
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (vkAllocateMemory(device_,
        &memAllocInfo, nullptr,
        &cubemap.textureInfo.texture.deviceMemory)
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate mem for cubemap image");
    }

    if (vkBindImageMemory(device_,
        cubemap.textureInfo.texture.image,
        cubemap.textureInfo.texture.deviceMemory, 0) 
        != VK_SUCCESS)
    {
        throw std::runtime_error("failed to bind mem for cubemap image");
    }

    VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = command_pool_;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer copyCmd{};
    if (vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, &copyCmd) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate copy comd");
    }

    VkCommandBufferBeginInfo cmdBufInfo{};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(copyCmd, &cmdBufInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin copy comd");
    }

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    uint32_t offset = 0;
    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t level = 0; level < cubemap.mipLevels; level++)
        {
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = level;
            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
            // TODO: check layerCount
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = texCube[face][level].extent().x;
            bufferCopyRegion.imageExtent.height = texCube[face][level].extent().y;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;

            bufferCopyRegions.push_back(bufferCopyRegion);

            // Increase offset into staging buffer for next level / face
            offset += texCube[face][level].size();
        }
    }

    // Image barrier for optimal image (target)
    // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = cubemap.mipLevels;
    subresourceRange.layerCount = 6;

    skybox_transitionLayout(
        copyCmd,
        cubemap.textureInfo.texture.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
    );

    vkCmdCopyBufferToImage(
        copyCmd,
        stagingBuffer,
        cubemap.textureInfo.texture.image,

        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data()
    );

    skybox_transitionLayout(
        copyCmd,
        cubemap.textureInfo.texture.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        subresourceRange,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
    );

    if (vkEndCommandBuffer(copyCmd) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to end copy comd");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;


    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;

    VkFence fence;
    if (vkCreateFence(device_, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create fence");
    }

    // Submit to the queue
    if (vkQueueSubmit(queue_, 1, &submitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit queue");
    }
    // Wait for the fence to signal that command buffer has finished executing
    if (vkWaitForFences(device_, 1, &fence, VK_TRUE, 100000000000))
    {
        throw std::runtime_error("failed to wait");

    }
    vkDestroyFence(device_, fence, nullptr);


    // create sampler
    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.maxAnisotropy = 1.0f;
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    sampler.maxLod = cubemap.mipLevels;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler.maxAnisotropy = 1.0f;
    //if (vulkanDevice->features.samplerAnisotropy)
    //{
    //	sampler.maxAnisotropy = vulkanDevice->properties.limits.maxSamplerAnisotropy;
    //	sampler.anisotropyEnable = VK_TRUE;
    //}
    if (vkCreateSampler(device_, &sampler, nullptr,
        &cubemap.textureInfo.texture.sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create cubemap sampler");
    }

    // create image view
    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    view.format = format;
    view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    // 6 array layers (faces)
    view.subresourceRange.layerCount = 6;
    view.subresourceRange.levelCount = cubemap.mipLevels;
    view.image = cubemap.textureInfo.texture.image;
    if (vkCreateImageView(device_, &view, nullptr, &cubemap.textureInfo.texture.imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create cubemap image view");

    }

    auto& descriptorImageInfo = cubemap.textureInfo.texture.descriptorImageInfo;
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo.imageView = cubemap.textureInfo.texture.imageView;
    descriptorImageInfo.sampler = cubemap.textureInfo.texture.sampler;

    // Clean up staging resources
    vkFreeMemory(device_, stagingMemory, nullptr);
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
}

void VulkanApp::loadSkyboxMesh() {
    skybox_.skyBoxCube.mesh.meshPath = "../../models/cube.obj";
    loadSingleSceneObjectMesh(skybox_.skyBoxCube.mesh);
}

void VulkanApp::createSkyboxUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(AppSkyBoxUniformBufferContent);
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        skybox_.uniformBufferAndContent.uniformBuffer.buffer,
        skybox_.uniformBufferAndContent.uniformBuffer.deviceMemory);

    VkDescriptorBufferInfo& buffer_info =
        skybox_.uniformBufferAndContent.uniformBuffer.descriptorBufferInfo;
    
    buffer_info.buffer =
        skybox_.uniformBufferAndContent.uniformBuffer.buffer;
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;
}

void VulkanApp::createSkyboxDescriptorSetLayout() {
    std::vector< VkDescriptorSetLayoutBinding> bindings = {
        // binding 0: uniform buf (cam)
        apputil::createDescriptorSetLayoutBinding(
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT),
        // binding 1: cube map texture
        apputil::createDescriptorSetLayoutBinding(
            1,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr,
        &skybox_.descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create skybox_.descriptorSetLayout!");
    }
}

void VulkanApp::createSkyboxDescriptorSet() {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &skybox_.descriptorSetLayout;

    auto& skybox_scene_object = skybox_.skyBoxCube.mesh;

    if (vkAllocateDescriptorSets(device_, &allocInfo,
        &skybox_scene_object.descriptorSet)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate deferred_.descriptorSet!");
    }

    std::vector<VkWriteDescriptorSet> write_sets = {
        // binding 0: uniform buf (cam)
        apputil::createBufferWriteDescriptorSet(
            skybox_scene_object.descriptorSet,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            0,
            &skybox_.uniformBufferAndContent.uniformBuffer
            .descriptorBufferInfo,
            1),
        // binding 1: cube map tex
        apputil::createImageWriteDescriptorSet(
            skybox_scene_object.descriptorSet,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            &skybox_.skyBoxCube.cubemap.textureInfo.texture.descriptorImageInfo,
            1)
    };

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(write_sets.size()),
        write_sets.data(), 0, NULL);
}

void VulkanApp::createSkyboxPipelineLayout() {
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo{};
    pPipelineLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.setLayoutCount = 1;
    pPipelineLayoutCreateInfo.pSetLayouts = &skybox_.descriptorSetLayout;

    if (vkCreatePipelineLayout(device_, &pPipelineLayoutCreateInfo, nullptr,
        &skybox_.pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed create skybox_.pipelineLayout");
    }
}

void VulkanApp::createSkyboxPipeline()
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState.flags = 0;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    // IMPT: check for back face
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.flags = 0;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.colorWriteMask = 0xf;
    blendAttachmentState.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.front = depthStencilState.back;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.flags = 0;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.flags = 0;

    std::vector<VkDynamicState> dynamicStateEnables = {
             VK_DYNAMIC_STATE_VIEWPORT,
             VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicState.flags = 0;

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
    shaderStages[0] = loadShader(
        "../../shaders/skybox.vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(
        "../../shaders/skybox.frag.spv",
        VK_SHADER_STAGE_FRAGMENT_BIT);

	std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates = {
			blendAttachmentState,
			blendAttachmentState,
			blendAttachmentState,
			blendAttachmentState

    };

    colorBlendState.attachmentCount =
        static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments = blendAttachmentStates.data();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = skybox_.pipelineLayout;
	// TODO: combine skybox and offscreen, so ues offscreen.rendepass
    // pipelineCreateInfo.renderPass = skybox_.renderPass;
	pipelineCreateInfo.renderPass = offscreen_.renderPass;

    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.pVertexInputState = &vertex_input_info.inputState;

    if (vkCreateGraphicsPipelines(device_, pipelineCache, 1,
        &pipelineCreateInfo, nullptr, &skybox_.pipeline)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create skybox_.pipeline");
    }
}


void VulkanApp::getEnabledFeatures()
{

    vkGetPhysicalDeviceFeatures(physical_device_, &device_features_);
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &device_memory_properties_);


    if (device_features_.samplerAnisotropy) {
        enabled_device_features_.samplerAnisotropy = VK_TRUE;
    }
    if (device_features_.textureCompressionBC) {
        enabled_device_features_.textureCompressionBC = VK_TRUE;
    }
    else if (device_features_.textureCompressionASTC_LDR) {
        enabled_device_features_.textureCompressionASTC_LDR = VK_TRUE;
    }
    else if (device_features_.textureCompressionETC2) {
        enabled_device_features_.textureCompressionETC2 = VK_TRUE;
    }
}


uint32_t VulkanApp::skybox_getMemoryType(uint32_t typeBits,
    VkMemoryPropertyFlags properties,
    VkBool32 *memTypeFound) {
    for (uint32_t i = 0; i < device_memory_properties_.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((device_memory_properties_.memoryTypes[i].propertyFlags & properties) == properties)
            {
                if (memTypeFound)
                {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if (memTypeFound)
    {
        *memTypeFound = false;
        return 0;
    }
    else
    {
        throw std::runtime_error("Could not find a matching memory type");
    }
}

void VulkanApp::skybox_transitionLayout(VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    // Create an image barrier object
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldImageLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source 
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newImageLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Put barrier inside setup command buffer
    vkCmdPipelineBarrier(
        cmdbuffer,
        srcStageMask,
        dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);
}

// deferred =================================================
void VulkanApp::prepareDeferred() {
    prepareQuadVertexAndIndexBuffer();
    createDeferredUniformBuffer();
    createDeferredDescriptorSetLayout();
    createDeferredDescriptorSet();
    createDeferredPipelineLayout();
    createDeferredRenderPass();
    createSwapChainFramebuffers();
	// TODO: move create pipeline to initVUlkan
     createDeferredPipeline();
    // TODO:
     createDeferredCommandBuffer();
}

void VulkanApp::createDeferredUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(AppDeferredUniformBufferContent);
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        deferred_.uniformBufferAndContent.uniformBuffer.buffer,
        deferred_.uniformBufferAndContent.uniformBuffer.deviceMemory
    );

    VkDescriptorBufferInfo& buffer_info =
        deferred_.uniformBufferAndContent.uniformBuffer.descriptorBufferInfo;

    buffer_info.buffer =
        deferred_.uniformBufferAndContent.uniformBuffer.buffer;
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;
}

void VulkanApp::createDeferredDescriptorSetLayout() {
    // all bindings are in fragment part, vert just passes UV
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        // binding 0: uniform buffer
        apputil::createDescriptorSetLayoutBinding(
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 1: position texture
        apputil::createDescriptorSetLayoutBinding(
            1,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 2: normal texture
        apputil::createDescriptorSetLayoutBinding(
            2,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 3: albedo texture
        apputil::createDescriptorSetLayoutBinding(
            3,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 4: Mrao texture
        apputil::createDescriptorSetLayoutBinding(
            4,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT),
		// binding 5: cube map texture
		apputil::createDescriptorSetLayoutBinding(
			5,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr,
        &deferred_.descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create deferred_.descriptorSetLayout!");
    }
}

void VulkanApp::createDeferredDescriptorSet() {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &deferred_.descriptorSetLayout;

    if (vkAllocateDescriptorSets(device_, &allocInfo, &deferred_.descriptorSet)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate deferred_.descriptorSet!");
    }

    // update this descriptorSet
    std::vector<VkWriteDescriptorSet> write_sets = {
        // binding 0: deferred uniform buffer
        apputil::createBufferWriteDescriptorSet(
            deferred_.descriptorSet,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            0,
            &deferred_.uniformBufferAndContent.uniformBuffer
            .descriptorBufferInfo,
            1),
        // binding 1: world position
        apputil::createImageWriteDescriptorSet(
            deferred_.descriptorSet,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            &offscreen_.frameBufferAssets.position.descriptorImageInfo,
            1),
        // binding 2: world normal
        apputil::createImageWriteDescriptorSet(
            deferred_.descriptorSet,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            2,
            &offscreen_.frameBufferAssets.normal.descriptorImageInfo,
            1),
        // binding 3: color
        apputil::createImageWriteDescriptorSet(
            deferred_.descriptorSet,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            3,
            &offscreen_.frameBufferAssets.color.descriptorImageInfo,
            1),
        // binding 4: mrao
        apputil::createImageWriteDescriptorSet(
            deferred_.descriptorSet,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            4,
            &offscreen_.frameBufferAssets.mrao.descriptorImageInfo,
            1)
    };

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(write_sets.size()),
        write_sets.data(), 0, NULL);
}

void VulkanApp::createDeferredPipelineLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &deferred_.descriptorSetLayout;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, nullptr,
        &deferred_.pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed at deferred_.pipelineLayout creation");
    }
}

void VulkanApp::createDeferredRenderPass() {
    // still black box here
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchain_imageformat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    // TODO do what?
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    subpass.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {
        colorAttachment, depthAttachment
    };

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr,
        &deferred_.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create deferred_.renderPass pass!");
    }
}

void VulkanApp::createDeferredPipeline() {
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState.flags = 0;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.flags = 0;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState blendAttachmentState{};
    blendAttachmentState.colorWriteMask = 0xf;
    blendAttachmentState.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.front = depthStencilState.back;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.flags = 0;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.flags = 0;

    std::vector<VkDynamicState> dynamicStateEnables = {
             VK_DYNAMIC_STATE_VIEWPORT,
             VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    dynamicState.flags = 0;

    VkPipelineVertexInputStateCreateInfo emptyInputState{};
    emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
    shaderStages[0] = loadShader(
        "../../shaders/deferred.vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(
        "../../shaders/deferred.frag.spv",
        VK_SHADER_STAGE_FRAGMENT_BIT);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = deferred_.pipelineLayout;
    pipelineCreateInfo.renderPass = deferred_.renderPass;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.pVertexInputState = &emptyInputState;

    if (vkCreateGraphicsPipelines(device_, pipelineCache, 1,
        &pipelineCreateInfo, nullptr, &deferred_.pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create deferred_.pipeline");
    }
}

void VulkanApp::createDeferredCommandBuffer() {
    deferred_command_buffers_.resize(swapchain_images_.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = command_pool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)deferred_command_buffers_.size();

    if (vkAllocateCommandBuffers(
        device_, &allocInfo, deferred_command_buffers_.data()) != VK_SUCCESS) {
        
        throw std::runtime_error(
            "failed to allocate deferred_command_buffer_s!");
    }

    for (size_t i = 0; i < deferred_command_buffers_.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = apputil::cmdBufferBegin(
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        if (vkBeginCommandBuffer(deferred_command_buffers_[i], &beginInfo)
            != VK_SUCCESS) {

            throw std::runtime_error(
                "failed to begin recording deferred_command_buffer_s!");
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = deferred_.renderPass;;
        renderPassInfo.framebuffer = swapchain_framebuffers_[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapchain_extent_;

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 0.3f, 0.0f, 0.3f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount =
            static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(deferred_command_buffers_[i], &renderPassInfo,
            VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = swapchain_extent_.width;
        viewport.height = swapchain_extent_.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(deferred_command_buffers_[i], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent.width = swapchain_extent_.width;
        scissor.extent.height = swapchain_extent_.height;
        scissor.offset.x = 0.0f;
        scissor.offset.y = 1.0f;
        vkCmdSetScissor(deferred_command_buffers_[i], 0, 1, &scissor);

        // VkBuffer vertexBuffers[] = { quadVertexBuffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindDescriptorSets(deferred_command_buffers_[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            deferred_.pipelineLayout, 0, 1,
            &deferred_.descriptorSet, 0, nullptr);

        vkCmdBindPipeline(deferred_command_buffers_[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS, deferred_.pipeline);
        vkCmdBindVertexBuffers(deferred_command_buffers_[i], 0, 1,
            &quadVertexBuffer, offsets);
        vkCmdBindIndexBuffer(deferred_command_buffers_[i], quadIndexBuffer, 0,
            VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(deferred_command_buffers_[i], static_cast<uint32_t>(6),
            1, 0, 0, 1);
        vkCmdEndRenderPass(deferred_command_buffers_[i]);

        if (vkEndCommandBuffer(deferred_command_buffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to end deferred_command_buffer_s!");
        }
    }
}

// general =================================================
void VulkanApp::draw_new() {
    //init submit info
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphore waitSemaphores[] = { semaphores_.presentComplete };

    mySubmitInfo.pWaitDstStageMask = waitStages;
    mySubmitInfo.signalSemaphoreCount = 1;
    mySubmitInfo.waitSemaphoreCount = 1;
    // wait for swap chian presentation to finish
    mySubmitInfo.pWaitSemaphores = waitSemaphores;
    mySubmitInfo.pSignalSemaphores = &offscreen_semaphore_;
    mySubmitInfo.commandBufferCount = 1;
    mySubmitInfo.pCommandBuffers = &offscreen_.commandBuffer;

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_,
        std::numeric_limits<uint64_t>::max(),
        semaphores_.presentComplete,
        VK_NULL_HANDLE, &imageIndex);

    // todo set wait semaphores and signal semaphores
    if (vkQueueSubmit(queue_, 1, &mySubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit queue 1");
    }

    // wait for offsreen
    VkSemaphore waitSemaphores_2[] = { offscreen_semaphore_ };
    mySubmitInfo.pWaitSemaphores = waitSemaphores_2;
    mySubmitInfo.pSignalSemaphores = &semaphores_.renderComplete;
    mySubmitInfo.pCommandBuffers = &deferred_command_buffers_[imageIndex];
    if (vkQueueSubmit(queue_, 1, &mySubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit deferred commmand buf");
    }

    VkResult res = queuePresent(queue_, imageIndex, semaphores_.renderComplete);
    vkQueueWaitIdle(queue_);
}

void VulkanApp::updateUniformBuffers() {
    // offscreen camera
    auto& ocs_ubo = offscreen_.uniformBufferAndContent.content;

    ocs_ubo.projMatrix = firstPersonCam->GetProj();
    ocs_ubo.viewMatrix = firstPersonCam->GetView();

    uniformBufferCpy(
        offscreen_.uniformBufferAndContent.uniformBuffer.deviceMemory, 
        &ocs_ubo, sizeof(ocs_ubo));   

    // scene object positions
    glm::mat4 modelMat;
    float time = GET_CLOBAL_TIME_GAP_SINCE_START();

    modelMat = glm::rotate(glm::mat4(1.0f),
        time * glm::radians(90.0f), glm::vec3(10.0f, 10.0f, 10.0f));
    auto& model0_ubo = scene_objects_[0].uniformBufferAndContent;
    model0_ubo.content.modelMatrix = modelMat;
    uniformBufferCpy(
        model0_ubo.uniformBuffer.deviceMemory,
        &model0_ubo.content, sizeof(model0_ubo.content));

    modelMat = glm::scale(glm::vec3(2.f, 2.f, 2.f));
    modelMat = glm::translate(glm::vec3(10.f, 10.f, 10.f));
    auto& model1_ubo = scene_objects_[1].uniformBufferAndContent;
    model1_ubo.content.modelMatrix = modelMat;
    uniformBufferCpy(
        model1_ubo.uniformBuffer.deviceMemory,
        &model1_ubo.content, sizeof(model1_ubo.content));


    
    // skybox
    auto& skybox_ubo = skybox_.uniformBufferAndContent;

    skybox_ubo.content.modelMatrix = getSkyboxModelMat();
    skybox_ubo.content.projMatrix = firstPersonCam->GetProj();
    skybox_ubo.content.viewMatrix = firstPersonCam->GetView();

    uniformBufferCpy(
        skybox_ubo.uniformBuffer.deviceMemory,
        &skybox_ubo.content, sizeof(skybox_ubo.content));
}

// helper
void VulkanApp::uniformBufferCpy(VkDeviceMemory& device_memory, void* ubo_ptr,
    size_t size) {
    void* data;
    vkMapMemory(device_, device_memory, 0, size, 0, &data);
    memcpy(data, ubo_ptr, size);
    vkUnmapMemory(device_, device_memory);
}

glm::mat4 VulkanApp::getSkyboxModelMat() {
    // skybox cam
    glm::mat4 modelMat = glm::mat4(1.0f);
    glm::vec3 forward = firstPersonCam->GetForward();
    modelMat = glm::rotate(modelMat, -forward.y,
        glm::vec3(1.0f, 0.0f, 0.0f));
    
    // TODO debug this 
    // std::cout << forward.x << " " << forward.y << std::endl;
    modelMat = glm::rotate(modelMat, forward.x,
        glm::vec3(0.0f, 1.0f, 0.0f));
    return modelMat;
}


// tryout: combine skybox and offscrenn =================================================

void VulkanApp::createOffscreenForSkyboxAndModel() {
	if (offscreen_.commandBuffer == VK_NULL_HANDLE)
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = command_pool_;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;
		if (vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, &offscreen_.commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create offscreenCommandBuffer");
		}
	}

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &offscreen_semaphore_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreenSemaphore");
	}

	VkCommandBufferBeginInfo cmdBufInfo{};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(offscreen_.commandBuffer, &cmdBufInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin offscreen_.commandBuffer");
	}

	// IMPT: to draw models, use viewport from 0 to n;
	float n_depth = 0.9999999f;
	VkViewport viewport{};
	viewport.width = swapchain_extent_.width;
	viewport.height = swapchain_extent_.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = n_depth;
	vkCmdSetViewport(offscreen_.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent.width = swapchain_extent_.width;
	scissor.extent.height = swapchain_extent_.height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(offscreen_.commandBuffer, 0, 1, &scissor);

	std::array<VkClearValue, 5> clearValues;
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[2].color = { { 1.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[4].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = offscreen_.renderPass;
	renderPassBeginInfo.framebuffer = offscreen_.frameBufferAssets.frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = swapchain_extent_.width;
	renderPassBeginInfo.renderArea.extent.height = swapchain_extent_.height;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(offscreen_.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkDeviceSize offsets[1] = { 0 };

	vkCmdBindPipeline(
		offscreen_.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		offscreen_.pipeline);

	// draw models
	for (auto& scene_object : scene_objects_) {
		vkCmdBindDescriptorSets(offscreen_.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS, offscreen_.pipelineLayout, 0, 1,
			&scene_object.descriptorSet, 0, NULL);

		vkCmdBindVertexBuffers(offscreen_.commandBuffer, 0, 1,
			&scene_object.vertexBuffer.buffer, offsets);

		vkCmdBindIndexBuffer(offscreen_.commandBuffer,
			scene_object.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(
			offscreen_.commandBuffer,
			static_cast<uint32_t>(scene_object.indexCount),
			1, 0, 0, 0);
	}

    // start draw sky box

	// IMPT: 
	// since the renderpass is only related to framebuffers and clearvalue
	// we use the same renderpass but different pipeline
	// 1. change viewport and scissor for command buffer
	VkViewport viewport_2{};
	viewport_2.width = swapchain_extent_.width;
	viewport_2.height = swapchain_extent_.height;
	viewport_2.minDepth = n_depth;
	viewport_2.maxDepth = 1.0f;
	vkCmdSetViewport(offscreen_.commandBuffer, 0, 1, &viewport_2);

	VkRect2D scissor_2{};
	scissor_2.extent.width = swapchain_extent_.width;
	scissor_2.extent.height = swapchain_extent_.height;
	scissor_2.offset.x = 0;
	scissor_2.offset.y = 0;
	vkCmdSetScissor(offscreen_.commandBuffer, 0, 1, &scissor_2);

	//2. change pipeline from offscreen from skybox
	bool use_skybox = true;
	if (use_skybox)
	{
		vkCmdBindPipeline(
			offscreen_.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			skybox_.pipeline);

		vkCmdBindDescriptorSets(offscreen_.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_.pipelineLayout, 0, 1,
			&skybox_.skyBoxCube.mesh.descriptorSet, 0, NULL);

		vkCmdBindVertexBuffers(offscreen_.commandBuffer, 0, 1,
			&skybox_.skyBoxCube.mesh.vertexBuffer.buffer, offsets);

		vkCmdBindIndexBuffer(offscreen_.commandBuffer,
			skybox_.skyBoxCube.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(
			offscreen_.commandBuffer,
			skybox_.skyBoxCube.mesh.indexCount,
			1, 0, 0, 0);
	}

	vkCmdEndRenderPass(offscreen_.commandBuffer);

	if (vkEndCommandBuffer(offscreen_.commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to end offscreenCommandBuffer");
	}
}

