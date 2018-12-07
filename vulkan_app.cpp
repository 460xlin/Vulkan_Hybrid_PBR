#include "vulkan_app.h"
#include "app_util.h"
#include <tiny_obj_loader.h>
#define TINYOBJLOADER_IMPLEMENTATION

#include <stb_image.h>

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
Camera* orbitCam;

void VulkanApp::initCam() {
    orbitCam = new Camera((float)HEIGHT / WIDTH);
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
        double sensitivity = 0.5;
        float deltaX = static_cast<float>((previousX - xPosition) * sensitivity);
        float deltaY = static_cast<float>((previousY - yPosition) * sensitivity);

        orbitCam->UpdateOrbit(deltaX, deltaY, 0.0f);

        previousX = xPosition;
        previousY = yPosition;
    }
    else if (rightMouseDown) {
        double deltaZ = static_cast<float>((previousY - yPosition) * 0.05);

        orbitCam->UpdateOrbit(0.0f, 0.0f, deltaZ);

        previousY = yPosition;
    }
}

void VulkanApp::keyDownCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    float timeGap = GET_CLOBAL_TIME_GAP_SINCE_LAST_RECORD();
    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        // go forward

    }
    else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        // go back

    }
    else if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        // go left

    }
    else if (key == GLFW_KEY_D && action == GLFW_PRESS) {
        // go right

    }
    else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        // go up
    }
    else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        // go down
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
    createScene();
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
    // begin offscreen
    // scene objects need offscreen's ubo, so has to be before object
    prepareSceneObjectsData();
    prepareOffscreen();
    prepareSceneObjectsDescriptor();
    prepareDeferred();

    // not related started -------------------------------------------------
    // createRenderPass();
    //createFramebuffers();
    //createDescriptorSetLayout();
    //createDescriptorSets_quad_old();

    //for (int i = 0; i < models.size(); ++i) {
    //    createTextureImage(models[i]);
    //    createTextureImageView(models[i]);
    //    createTextureSampler(models[i]);
    //}
    //
    //createHardCodeModelVertexBuffer();
    //createHardCodeModelIndexBuffer();

    //loadModel();

    //createUniformBuffers();

    //
    //rt_graphics_setupDescriptorSetLayout();
    //
    //// hader uses descriptor slot 0.0 error
    //createGraphicsPipeline_old();
    //createDeferredCommandBuffers_old();
    ////createOffscreenCommandBuffer();

    //createSyncObjects();

    //rt_prepareStorageBuffers();
    //rt_prepareTextureTarget(rt_result, VK_FORMAT_R8G8B8A8_UNORM);
    //// rt_graphics_setupDescriptorSetLayout();
    ////  Unable to allocate 1 descriptorSets from pool error
    //rt_graphics_setupDescriptorSet();
    //rt_prepareCompute();
    //rt_createComputeCommandBuffer();

    //rt_createRaytraceDisplayCommandBuffer();
}

void VulkanApp::mainLoop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        // rt_draw();
        // draw();
        draw_new();
    }

    vkDeviceWaitIdle(device_);
}

void VulkanApp::cleanupSwapChain() {
    vkDestroyImageView(device_, depth_attachment_.imageView, nullptr);
    vkDestroyImage(device_, depth_attachment_.image, nullptr);
    vkFreeMemory(device_, depth_attachment_.deviceMemory, nullptr);

    for (auto framebuffer : swapchain_framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    vkFreeCommandBuffers(device_, command_pool_, static_cast<uint32_t>(command_buffers_.size()), command_buffers_.data());

    vkDestroyPipelineLayout(device_, deferredPipelineLayout, nullptr);
    //vkDestroyPipelineLayout(device_, offscreenPipelineLayout, nullptr);

    vkDestroyRenderPass(device_, deferred_renderpass_, nullptr);

    for (auto imageView : swapchain_imageviews_) {
        vkDestroyImageView(device_, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
}

void VulkanApp::cleanup() {

    vkDestroySampler(device_, colorSampler, nullptr);

    for (auto shaderModule : shaderModules) {
        vkDestroyShaderModule(device_, shaderModule, nullptr);
    }

    cleanupSwapChain();

    for ( auto& model: models )
    {
        vkDestroySampler(device_, model.albedoTexture.textureSampler, nullptr);
        vkDestroyImageView(device_, model.albedoTexture.textureImageView, nullptr);

        vkDestroyImage(device_, model.albedoTexture.textureImage, nullptr);
        vkFreeMemory(device_, model.albedoTexture.textureImageMemory, nullptr);

        vkDestroySampler(device_, model.normalMapTexture.textureSampler, nullptr);
        vkDestroyImageView(device_, model.normalMapTexture.textureImageView, nullptr);

        vkDestroyImage(device_, model.normalMapTexture.textureImage, nullptr);
        vkFreeMemory(device_, model.normalMapTexture.textureImageMemory, nullptr);
    }
    

    vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);

    vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);


    vkDestroyBuffer(device_, uniformBuffers.vsFullScreen.buffer, nullptr);
    vkFreeMemory(device_, uniformBuffers.vsFullScreen.deviceMem, nullptr);

    //vkDestroyBuffer(device_, uniformBuffers.vsOffScreen.buffer, nullptr);
    //vkFreeMemory(device_, uniformBuffers.vsOffScreen.deviceMem, nullptr);

    vkDestroyBuffer(device_, quadIndexBuffer, nullptr);
    vkFreeMemory(device_, quadIndexBufferMemory, nullptr);

    vkDestroyBuffer(device_, quadVertexBuffer, nullptr);
    vkFreeMemory(device_, quadVertexBufferMemory, nullptr);

    vkDestroySemaphore(device_, offscreenSemaphore, nullptr);


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

    delete orbitCam;
}

void VulkanApp::recreateSwapChain() {
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window_, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device_);

    cleanupSwapChain();

    createSwapChain();
    createSwapChainImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createDeferredCommandBuffers_old();
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

void VulkanApp::createRenderPass() {
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
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    // TODO 
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

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());;
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &deferred_renderpass_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create deferredRenderPass pass!");
    }

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &graphics.rt_rayTraceRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics.rt_rayTraceRenderPass pass!");
    }
}

void VulkanApp::createDescriptorSetLayout() {
    // Binding 0 : Vertex shader uniform buffer
    VkDescriptorSetLayoutBinding vsUboLayoutBinding = {};
    vsUboLayoutBinding.binding = 0;
    vsUboLayoutBinding .descriptorCount = 1;
    vsUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vsUboLayoutBinding.pImmutableSamplers = nullptr;
    vsUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Binding 1 : Position texture target / Scene colormap
    VkDescriptorSetLayoutBinding positionLayoutBinding = {};
    positionLayoutBinding.binding = 1;
    positionLayoutBinding.descriptorCount = 1;
    positionLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    positionLayoutBinding.pImmutableSamplers = nullptr;
    positionLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Binding 2 : Normals texture target
    VkDescriptorSetLayoutBinding normalLayoutBinding = {};
    normalLayoutBinding.binding = 2;
    normalLayoutBinding.descriptorCount = 1;
    normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalLayoutBinding.pImmutableSamplers = nullptr;
    normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Binding 3 : Albedo texture target
    VkDescriptorSetLayoutBinding albedoLayoutBinding = {};
    albedoLayoutBinding.binding = 3;
    albedoLayoutBinding.descriptorCount = 1;
    albedoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    albedoLayoutBinding.pImmutableSamplers = nullptr;
    albedoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Binding 4 : Fragment shader uniform buffer
    VkDescriptorSetLayoutBinding fsUboLayoutBinding = {};
    fsUboLayoutBinding.binding = 4;
    fsUboLayoutBinding.descriptorCount = 1;
    fsUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    fsUboLayoutBinding.pImmutableSamplers = nullptr;
    fsUboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vsUboLayoutBinding,
        positionLayoutBinding,
        normalLayoutBinding,
        albedoLayoutBinding,
        fsUboLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptor_set_layout_;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, nullptr, &deferredPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed at deferredPipelineLayout creation");
    }
}
void VulkanApp::createGraphicsPipeline() {
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

    //create pipeline, prepare pipeline create info



    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = deferredPipelineLayout;
    pipelineCreateInfo.renderPass = deferred_renderpass_;
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

    // Empty vertex input state, quads are generated by the vertex shader

    VkPipelineVertexInputStateCreateInfo emptyInputState{};
    emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    pipelineCreateInfo.pVertexInputState = &emptyInputState;
    pipelineCreateInfo.layout = deferredPipelineLayout;



    // ray tracing pipeline
    // change rasterization cull mode depth not enabled
    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // depth not enabled
    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;
    //change color blend state
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;
    // reuse input state
    emptyInputState.vertexAttributeDescriptionCount = 0;
    emptyInputState.pVertexAttributeDescriptions = nullptr;
    emptyInputState.vertexBindingDescriptionCount = 0;
    emptyInputState.pVertexBindingDescriptions = nullptr;

    shaderStages[0] = loadShader("C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader("C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);


    pipelineCreateInfo.pVertexInputState = &emptyInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = shaderStages.size();
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.renderPass = deferred_renderpass_;

    if (vkCreateGraphicsPipelines(device_, pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphics.rt_raytracePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create rt_raytracePipeline");
    }
}
void VulkanApp::createGraphicsPipeline_old() {
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

    //create pipeline, prepare pipeline create info



    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = deferredPipelineLayout;
    pipelineCreateInfo.renderPass = deferred_renderpass_;
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

    // final fullscreen
    
    shaderStages[0] = loadShader("C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/deferred.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader("C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/deferred.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    // Empty vertex input state, quads are generated by the vertex shader

    VkPipelineVertexInputStateCreateInfo emptyInputState{};
    emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    pipelineCreateInfo.pVertexInputState = &emptyInputState;
    pipelineCreateInfo.layout = deferredPipelineLayout;


    if (vkCreateGraphicsPipelines(device_, pipelineCache, 1, &pipelineCreateInfo, nullptr, &deferredPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create deferredPipeline");
    }
 

    // Offscreen pipeline
    shaderStages[0] = loadShader("C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/mrt.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader("C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/mrt.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    // change layout and render pass
    pipelineCreateInfo.pVertexInputState = &vertices_new.inputState;
    pipelineCreateInfo.renderPass = offscreen_.renderPass;
    pipelineCreateInfo.layout = offscreen_.pipelineLayout;

    std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
            blendAttachmentState,
            blendAttachmentState,
            blendAttachmentState
    };

    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments = blendAttachmentStates.data();
    if (vkCreateGraphicsPipelines(device_, pipelineCache, 1, &pipelineCreateInfo, nullptr, &offscreen_.pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create offscreenPipeline");
    }

    // ray tracing pipeline
    // change rasterization cull mode depth not enabled
    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // depth not enabled
    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;
    //change color blend state
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;
    // reuse input state
    emptyInputState.vertexAttributeDescriptionCount = 0;
    emptyInputState.pVertexAttributeDescriptions = nullptr;
    emptyInputState.vertexBindingDescriptionCount = 0;
    emptyInputState.pVertexBindingDescriptions = nullptr;

    shaderStages[0] = loadShader("C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader("C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);


    pipelineCreateInfo.pVertexInputState = &emptyInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = shaderStages.size();
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.renderPass = deferred_renderpass_;
    pipelineCreateInfo.layout = graphics.rt_raytracePipelineLayout;

    if (vkCreateGraphicsPipelines(device_, pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphics.rt_raytracePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create rt_raytracePipeline");
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


void VulkanApp::createFramebuffers() {
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

void VulkanApp::createTextureImage(ModelVertexIndexTextureBuffer& model) {

    // albedo texture 
    int albedo_texWidth, albedo_texHeight, albedo_texChannels;
    stbi_uc* albedo_pixels = stbi_load(model.paths.albedoTexturePath.c_str(), &albedo_texWidth, &albedo_texHeight, &albedo_texChannels, STBI_rgb_alpha);
    VkDeviceSize albedo_imageSize = albedo_texWidth * albedo_texHeight * 4;

    if (!albedo_pixels) {
        throw std::runtime_error("failed to load abledo texture image!");
    }

    VkBuffer albedo_stagingBuffer;
    VkDeviceMemory albedo_stagingBufferMemory;

    createBuffer(albedo_imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        albedo_stagingBuffer, albedo_stagingBufferMemory);

    void* data;
    vkMapMemory(device_, albedo_stagingBufferMemory, 0, albedo_imageSize, 0, &data);
    memcpy(data, albedo_pixels, static_cast<size_t>(albedo_imageSize));
    vkUnmapMemory(device_, albedo_stagingBufferMemory);

    stbi_image_free(albedo_pixels);

    createImage(albedo_texWidth, albedo_texHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        model.albedoTexture.textureImage, model.albedoTexture.textureImageMemory);

    transitionImageLayout(model.albedoTexture.textureImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(albedo_stagingBuffer, model.albedoTexture.textureImage, 
        static_cast<uint32_t>(albedo_texWidth), static_cast<uint32_t>(albedo_texHeight));
    transitionImageLayout(model.albedoTexture.textureImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device_, albedo_stagingBuffer, nullptr);
    vkFreeMemory(device_, albedo_stagingBufferMemory, nullptr);


    // normalMap texture 
    int nor_texWidth, nor_texHeight, nor_texChannels;
    stbi_uc* nor_pixels = stbi_load(model.paths.normalTexturePath.c_str(), &nor_texWidth, &nor_texHeight, &nor_texChannels, STBI_rgb_alpha);
    VkDeviceSize nor_imageSize = nor_texWidth * nor_texHeight * 4;

    if (!nor_pixels) {
        throw std::runtime_error("failed to load abledo texture image!");
    }

    VkBuffer nor_stagingBuffer;
    VkDeviceMemory nor_stagingBufferMemory;

    createBuffer(nor_imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        nor_stagingBuffer, nor_stagingBufferMemory);

    void* normalData;

    vkMapMemory(device_, nor_stagingBufferMemory, 0, nor_imageSize, 0, &normalData);
    memcpy(normalData, nor_pixels, static_cast<size_t>(nor_imageSize));
    vkUnmapMemory(device_, nor_stagingBufferMemory);

    stbi_image_free(nor_pixels);

    createImage(nor_texWidth, nor_texHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        model.normalMapTexture.textureImage, model.normalMapTexture.textureImageMemory);

    transitionImageLayout(model.normalMapTexture.textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(nor_stagingBuffer, model.normalMapTexture.textureImage, static_cast<uint32_t>(nor_texWidth), static_cast<uint32_t>(nor_texHeight));
    transitionImageLayout(model.normalMapTexture.textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device_, nor_stagingBuffer, nullptr);
    vkFreeMemory(device_, nor_stagingBufferMemory, nullptr);
}

void VulkanApp::createTextureImageView(ModelVertexIndexTextureBuffer& model) {
    model.albedoTexture.textureImageView = createImageView(model.albedoTexture.textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    model.normalMapTexture.textureImageView = createImageView(model.normalMapTexture.textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanApp::createTextureSampler(ModelVertexIndexTextureBuffer& model) {

    // TODO:  maybe we can improve the accurancy of normal map
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

    if (vkCreateSampler(device_, &samplerInfo, nullptr, &model.albedoTexture.textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create albedo texture sampler!");
    }

    if (vkCreateSampler(device_, &samplerInfo, nullptr, &model.normalMapTexture.textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create normalMap texture sampler!");
    }
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

void VulkanApp::loadModel() {

    loadArbitaryModelAndBuffers(models[1]);
}

void VulkanApp::createHardCodeModelVertexBuffer() {

    struct Vertex {
        float pos[3];
        float uv[2];
        float col[3];
        float normal[3];
        float tangent[3];
    };

    std::vector<Vertex> tempVertexBuffer;

    tempVertexBuffer.push_back({ {0.f, 0.f, 0.0f}, { 1.0f, 0.0f }, {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f, 0.f} });
    tempVertexBuffer.push_back({ {5.f, 0.f, 0.0f}, {0.0f, 0.0f}, {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 0.f} });
    tempVertexBuffer.push_back({ {5.f, 5.f, 0.0f}, {0.0f, 1.0f}, {1.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, {1.f, 0.f, 1.f} });
    tempVertexBuffer.push_back({ {0.f, 5.f, 0.0f}, {1.0f, 1.0f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f, 0.f} });

    //tempVertexBuffer.push_back({ {-5.f, -5.f, 5.f}, {1.0f, 0.0f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f} });
    //tempVertexBuffer.push_back({ {5.f, -5.f, 5.f}, {0.0f, 0.0f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f, 0.f} });
    //tempVertexBuffer.push_back({ {5.f, 5.f, 5.f}, {0.0f, 1.0f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f} });
    //tempVertexBuffer.push_back({ {-5.f, 5.f, 5.f}, {1.0f, 1.0f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f, 0.f} });

    VkDeviceSize bufferSize = sizeof(Vertex) * tempVertexBuffer.size();

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
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, models[0].vertexBuffer, models[0].vertexMem);

    copyBuffer(stagingBuffer, models[0].vertexBuffer, bufferSize);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

void VulkanApp::loadSceneObjectMesh(AppSceneObject& object_struct) {
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

    for (const auto& shape : shapes) {
        // +3 for per triangle
        for (int i = 0; i < shape.mesh.indices.size(); i += 3) {
            Vertex verts[3];
            const tinyobj::index_t idx[] = {
                shape.mesh.indices[i],
                shape.mesh.indices[i + 1],
                shape.mesh.indices[i + 2]};

            for (int j = 0; j < 3; ++j) {
                const auto& index = idx[j];
                auto& vert = verts[j];
                vert.pos[0] = attrib.vertices[3 * index.vertex_index + 2];
                vert.pos[1] = attrib.vertices[3 * index.vertex_index + 1];
                vert.pos[2] = attrib.vertices[3 * index.vertex_index + 0];

                vert.uv[0] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 0];
                vert.uv[1] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];

                vert.col[0] = 0.f;
                vert.col[1] = 0.f;
                vert.col[2] = 1.f;

                vert.normal[0] = attrib.normals[3 * index.vertex_index + 2];
                vert.normal[1] = attrib.normals[3 * index.vertex_index + 1];
                vert.normal[2] = attrib.normals[3 * index.vertex_index + 0];
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

        //for (const auto& index : shape.mesh.indices) {
        //    Vertex vertex = {};

        //    vertex.pos[0] = attrib.vertices[3 * index.vertex_index + 2];
        //    vertex.pos[1] = attrib.vertices[3 * index.vertex_index + 1];
        //    vertex.pos[2] = attrib.vertices[3 * index.vertex_index + 0];

        //    // IMPT
        //    /*vertex.uv[0] = attrib.texcoords[2 * index.texcoord_index + 0];
        //    vertex.uv[1] = attrib.texcoords[2 * index.texcoord_index + 1];*/

        //    vertex.uv[0] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 0];
        //    vertex.uv[1] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];

        //    vertex.col[0] = 1.f;
        //    vertex.col[1] = 0.f;
        //    vertex.col[2] = 1.f;

        //    vertex.normal[0] = attrib.normals[3 * index.vertex_index + 2];
        //    vertex.normal[1] = attrib.normals[3 * index.vertex_index + 1];
        //    vertex.normal[2] = attrib.normals[3 * index.vertex_index + 0];

        //    vertex.tangent[0] = 0.3f;
        //    vertex.tangent[1] = 0.6f;
        //    vertex.tangent[2] = 0.3f;

        //    vertices.push_back(vertex);
        //    indices.push_back(indices.size());
        //}
    }
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

void VulkanApp::loadArbitaryModelAndBuffers(ModelVertexIndexTextureBuffer& model_struct) {
    struct Vertex {
        float pos[3];
        float uv[2];
        float col[3];
        float normal[3];
        float tangent[3];
    };

    std::string file_path = model_struct.paths.modelPath;

    std::vector<Vertex> tempVertexBuffer;
    std::vector<uint32_t> tempIndexBuffer;


    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    // std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};

            vertex.pos[0] = attrib.vertices[3 * index.vertex_index + 2];
            vertex.pos[1] = attrib.vertices[3 * index.vertex_index + 1];
            vertex.pos[2] = attrib.vertices[3 * index.vertex_index + 0];

            // IMPT
            /*vertex.uv[0] = attrib.texcoords[2 * index.texcoord_index + 0];
            vertex.uv[1] = attrib.texcoords[2 * index.texcoord_index + 1];*/

            vertex.uv[0] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 0];
            vertex.uv[1] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];

            vertex.col[0] = 1.f;
            vertex.col[1] = 0.f;
            vertex.col[2] = 1.f;

            vertex.normal[0] = attrib.normals[3 * index.vertex_index + 2];
            vertex.normal[1] = attrib.normals[3 * index.vertex_index + 1];
            vertex.normal[2] = attrib.normals[3 * index.vertex_index + 0];

            vertex.tangent[0] = 0.3f;
            vertex.tangent[1] = 0.6f;
            vertex.tangent[2] = 0.3f;

            tempVertexBuffer.push_back(vertex);
            tempIndexBuffer.push_back(tempIndexBuffer.size());
        }
    }
    model_struct.vertexCount = static_cast<uint32_t>(tempVertexBuffer.size());
    model_struct.indexCount = static_cast<uint32_t>(tempIndexBuffer.size());

    // create vertex buffer for arbitary  model
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * tempVertexBuffer.size();

    VkBuffer vertexStagingBuffer;
    VkDeviceMemory vertexStagingBufferMemory;
    createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexStagingBuffer, vertexStagingBufferMemory);

    void* vertexData;
    vkMapMemory(device_, vertexStagingBufferMemory, 0, vertexBufferSize, 0, &vertexData);
    memcpy(vertexData, tempVertexBuffer.data(), (size_t)vertexBufferSize);
    vkUnmapMemory(device_, vertexStagingBufferMemory);

    createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        model_struct.vertexBuffer,
        model_struct.vertexMem
    );

    copyBuffer(vertexStagingBuffer, model_struct.vertexBuffer, vertexBufferSize);

    vkDestroyBuffer(device_, vertexStagingBuffer, nullptr);
    vkFreeMemory(device_, vertexStagingBufferMemory, nullptr);

    // create index buffer for arbitary  model

    VkDeviceSize indexBufferSize = sizeof(tempIndexBuffer[0]) * tempIndexBuffer.size();

    VkBuffer indexStagingBuffer;
    VkDeviceMemory indexStagingBufferMemory;
    createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexStagingBuffer, indexStagingBufferMemory);

    void* indexData;
    vkMapMemory(device_, indexStagingBufferMemory, 0, indexBufferSize, 0, &indexData);
    memcpy(indexData, tempIndexBuffer.data(), (size_t)indexBufferSize);
    vkUnmapMemory(device_, indexStagingBufferMemory);

    createBuffer(indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, model_struct.indexBuffer, model_struct.indexMem);

    copyBuffer(indexStagingBuffer, model_struct.indexBuffer, indexBufferSize);

    vkDestroyBuffer(device_, indexStagingBuffer, nullptr);
    vkFreeMemory(device_, indexStagingBufferMemory, nullptr);
}

void VulkanApp::createHardCodeModelIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices_.data(), (size_t)bufferSize);
    vkUnmapMemory(device_, stagingBufferMemory);

    createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, models[0].indexBuffer, models[0].indexMem);

    copyBuffer(stagingBuffer, models[0].indexBuffer, bufferSize);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

void VulkanApp::createUniformBuffers() {
    VkDeviceSize deferredBufferSize = sizeof(DeferredUniformBufferObject);
    // TODO: CHECK if both vsFullScrenn and vsOffScreen use same uniform buffer data
    // not the same
    createBuffer(deferredBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        uniformBuffers.vsFullScreen.buffer, uniformBuffers.vsFullScreen.deviceMem
    );
    
    

    VkDeviceSize rtBufferSize = sizeof(RTUniformBufferObject);
    createBuffer(rtBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        uniformBuffers.rt_compute.buffer, uniformBuffers.rt_compute.deviceMem
    );
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

void VulkanApp::createDescriptorSets_quad_old() {
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptor_set_layout_;

    if (vkAllocateDescriptorSets(device_, &allocInfo, &quadDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate quad descriptor sets!");
    }

    VkDescriptorImageInfo texPositionDesc = {};
    texPositionDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texPositionDesc.sampler = colorSampler;
    texPositionDesc.imageView = offscreen_.frameBufferAssets.position.imageView;

    VkDescriptorImageInfo texNormalDesc = {};
    texNormalDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texNormalDesc.sampler = colorSampler;
    texNormalDesc.imageView = offscreen_.frameBufferAssets.normal.imageView;

    VkDescriptorImageInfo texAlbedoDesc = {};
    texAlbedoDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texAlbedoDesc.sampler = colorSampler;
    texAlbedoDesc.imageView = offscreen_.frameBufferAssets.color.imageView;

    // TODO: where to define buffer and buffer info for descriptor????????????????
    uniformBuffers.vsFullScreen.desBuffInfo.buffer = uniformBuffers.vsFullScreen.buffer;
    uniformBuffers.vsFullScreen.desBuffInfo.offset = 0;
    uniformBuffers.vsFullScreen.desBuffInfo.range = sizeof(uniformBuffers.vsFullScreen);

    VkWriteDescriptorSet uniformVsFullWriteDesSet{};
    uniformVsFullWriteDesSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uniformVsFullWriteDesSet.dstSet = quadDescriptorSet;
    uniformVsFullWriteDesSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformVsFullWriteDesSet.dstBinding = 0;
    uniformVsFullWriteDesSet.pBufferInfo = &uniformBuffers.vsFullScreen.desBuffInfo;
    uniformVsFullWriteDesSet.descriptorCount = 1;

    VkWriteDescriptorSet positionWriteDesSet{};
    positionWriteDesSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    positionWriteDesSet.dstSet = quadDescriptorSet;
    positionWriteDesSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    positionWriteDesSet.dstBinding = 1;
    positionWriteDesSet.pImageInfo = &texPositionDesc;
    positionWriteDesSet.descriptorCount = 1;

    VkWriteDescriptorSet normalWriteDesSet{};
    normalWriteDesSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    normalWriteDesSet.dstSet = quadDescriptorSet;
    normalWriteDesSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalWriteDesSet.dstBinding = 2;
    normalWriteDesSet.pImageInfo = &texNormalDesc;
    normalWriteDesSet.descriptorCount = 1;

    VkWriteDescriptorSet albedoWriteDesSet{};
    albedoWriteDesSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    albedoWriteDesSet.dstSet = quadDescriptorSet;
    albedoWriteDesSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    albedoWriteDesSet.dstBinding = 3;
    albedoWriteDesSet.pImageInfo = &texAlbedoDesc;
    albedoWriteDesSet.descriptorCount = 1;

    VkWriteDescriptorSet tempWriteDesSet{};
    tempWriteDesSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    tempWriteDesSet.dstSet = quadDescriptorSet;
    tempWriteDesSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    tempWriteDesSet.dstBinding = 4;
    tempWriteDesSet.pBufferInfo = &uniformBuffers.vsFullScreen.desBuffInfo;
    tempWriteDesSet.descriptorCount = 1;

    // TODO: add multiple light source
    //VkWriteDescriptorSet uniformVsFullWriteDesSet{};
    //uniformVsFullWriteDesSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //uniformVsFullWriteDesSet.dstSet = quadDescriptorSet;
    //uniformVsFullWriteDesSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //uniformVsFullWriteDesSet.dstBinding = 0;
    //uniformVsFullWriteDesSet.pBufferInfo = &uniformBuffers.vsFullScreen.desBuffInfo;
    //uniformVsFullWriteDesSet.descriptorCount = 1;

    // binding 0: Vertex shader uniform buffer
    // Binding 1 : Position texture target
    // Binding 2 : Normals texture target
    // Binding 3 : Albedo texture target
    // Binding 4 : temp texture target : for debug!!!!
    // TODO!
    writeDescriptorSets = { uniformVsFullWriteDesSet, positionWriteDesSet, normalWriteDesSet, albedoWriteDesSet, tempWriteDesSet };
    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);



    // for mrt shaders and it is for off screen rendering
    //for (int i = 0; i < models.size(); ++i) {
    //    ModelVertexIndexTextureBuffer& curModel = models[i];

    //    if (vkAllocateDescriptorSets(device_, &allocInfo, &curModel.descriptorSet) != VK_SUCCESS)
    //    {
    //        throw std::runtime_error("failed to allocate modelDesctiptorSets.model_1");
    //    }


    //    VkDescriptorBufferInfo temp{};
    //    temp.buffer = offscreen_.cameraUniformBuffer.buffer;
    //    temp.offset = 0;
    //    temp.range = VK_WHOLE_SIZE;

    //    VkDescriptorImageInfo model_1_albebo = {};
    //    model_1_albebo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //    model_1_albebo.sampler = colorSampler;
    //    model_1_albebo.imageView = curModel.albedoTexture.textureImageView;

    //    VkDescriptorImageInfo model_1_normal = {};
    //    model_1_normal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //    model_1_normal.sampler = curModel.normalMapTexture.textureSampler;
    //    model_1_normal.imageView = curModel.normalMapTexture.textureImageView;

    //    

    //    offscreen_.cameraUniformBuffer.descriptorBufferInfo.buffer = offscreen_.cameraUniformBuffer.buffer;
    //    offscreen_.cameraUniformBuffer.descriptorBufferInfo.offset = 0;
    //    offscreen_.cameraUniformBuffer.descriptorBufferInfo.range = VK_WHOLE_SIZE;




    //    VkWriteDescriptorSet model_writeDesSet{};
    //    model_writeDesSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //    model_writeDesSet.dstSet = curModel.descriptorSet;
    //    model_writeDesSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //    model_writeDesSet.dstBinding = 0;
    //    model_writeDesSet.pBufferInfo = &offscreen_.cameraUniformBuffer.descriptorBufferInfo;
    //    model_writeDesSet.descriptorCount = 1;

    //    // todo: match new descriptor layout 
    //    VkWriteDescriptorSet tempWrite{};
    //    tempWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //    tempWrite.dstSet = curModel.descriptorSet;
    //    tempWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //    tempWrite.dstBinding = 0;
    //    tempWrite.pBufferInfo = &temp;
    //    tempWrite.descriptorCount = 1;

    //    VkWriteDescriptorSet model_albedoWriteDesSet{};
    //    model_albedoWriteDesSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //    model_albedoWriteDesSet.dstSet = curModel.descriptorSet;
    //    model_albedoWriteDesSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //    model_albedoWriteDesSet.dstBinding = 1;
    //    model_albedoWriteDesSet.pImageInfo = &model_1_albebo;
    //    model_albedoWriteDesSet.descriptorCount = 1;

    //    VkWriteDescriptorSet model_normalWriteDesSet{};
    //    model_normalWriteDesSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //    model_normalWriteDesSet.dstSet = curModel.descriptorSet;
    //    model_normalWriteDesSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //    model_normalWriteDesSet.dstBinding = 2;
    //    model_normalWriteDesSet.pImageInfo = &model_1_normal;
    //    model_normalWriteDesSet.descriptorCount = 1;

    //    // those are zero
    //    std::cout << "color-------------" << std::endl;
    //    std::cout << model_1_albebo.imageView << std::endl;
    //    std::cout << "nor---------------" << std::endl;
    //    std::cout << model_1_normal.imageView << std::endl;

    //    std::vector<VkWriteDescriptorSet> model_writeDescriptorSets 
    //    { model_writeDesSet, tempWrite, model_albedoWriteDesSet, model_normalWriteDesSet };

    //    //model_writeDescriptorSets = { model_writeDesSet, model_albedoWriteDesSet, model_normalWriteDesSet };
    //    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(model_writeDescriptorSets.size()),
    //        model_writeDescriptorSets.data(), 0, NULL);
    //}

    // TODO: for ray tracing
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

// todo check whether the buffer of model is passed into the command buffer
void VulkanApp::createDeferredCommandBuffers_old() {
    command_buffers_.resize(swapchain_framebuffers_.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = command_pool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)command_buffers_.size();

    if (vkAllocateCommandBuffers(device_, &allocInfo, command_buffers_.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < command_buffers_.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        if (vkBeginCommandBuffer(command_buffers_[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = deferred_renderpass_;
        renderPassInfo.framebuffer = swapchain_framebuffers_[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapchain_extent_;

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 0.3f, 0.0f, 0.3f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(command_buffers_[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


        VkViewport viewport{};
        viewport.width = swapchain_extent_.width;
        viewport.height = swapchain_extent_.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffers_[i], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent.width = swapchain_extent_.width;
        scissor.extent.height = swapchain_extent_.height;
        scissor.offset.x = 0.0f;
        scissor.offset.y = 1.0f;
        vkCmdSetScissor(command_buffers_[i], 0, 1, &scissor);

        VkBuffer vertexBuffers[] = { quadVertexBuffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindDescriptorSets(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipelineLayout, 0, 1, &quadDescriptorSet, 0, nullptr);

        vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipeline);
        vkCmdBindVertexBuffers(command_buffers_[i], 0, 1, &quadVertexBuffer, offsets);
        vkCmdBindIndexBuffer(command_buffers_[i], quadIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffers_[i], static_cast<uint32_t>(6), 1, 0, 0, 1);
        vkCmdEndRenderPass(command_buffers_[i]);

        if (vkEndCommandBuffer(command_buffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

/*void VulkanApp::createOffscreenCommandBuffer() {
    if (offscreen_.frameBufferAssets == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo {};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = command_pool_;
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocateInfo.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, &offscreenCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create offscreenCommandBuffer");
        }
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &offscreenSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("failed to create offscreenSemaphore");
    }

    
    VkCommandBufferBeginInfo cmdBufInfo{};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    std::array<VkClearValue, 4> clearValues;
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[3].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = offScreenFrameBuf.renderPass;
    renderPassBeginInfo.framebuffer = offScreenFrameBuf.frameBuffer;
    renderPassBeginInfo.renderArea.extent.width = swapchain_extent_.width;
    renderPassBeginInfo.renderArea.extent.height = swapchain_extent_.height;
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    if (vkBeginCommandBuffer(offscreenCommandBuffer, &cmdBufInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin offscreenCommandBuffer");
    }

    vkCmdBeginRenderPass(offscreenCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width = swapchain_extent_.width;
    viewport.height = swapchain_extent_.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(offscreenCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent.width = swapchain_extent_.width;
    scissor.extent.height = swapchain_extent_.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(offscreenCommandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline);

    VkDeviceSize offsets[1] = { 0 };

    // Draw model 0
    vkCmdBindDescriptorSets(offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        offscreenPipelineLayout, 0, 1, &models[0].descriptorSet, 0, NULL);
    vkCmdBindVertexBuffers(offscreenCommandBuffer, 0, 1, &models[0].vertexBuffer, offsets);
    vkCmdBindIndexBuffer(offscreenCommandBuffer, models[0].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(offscreenCommandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    // the first number is indinstance count
    // Draw model 1
    vkCmdBindDescriptorSets(offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        offscreenPipelineLayout, 0, 1, &models[1].descriptorSet, 0, NULL);
    vkCmdBindVertexBuffers(offscreenCommandBuffer, 0, 1, &models[1].vertexBuffer, offsets);
    vkCmdBindIndexBuffer(offscreenCommandBuffer, models[1].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(offscreenCommandBuffer, models[1].indexCount, 4, 0, 0, 0);

    vkCmdEndRenderPass(offscreenCommandBuffer);

    if (vkEndCommandBuffer(offscreenCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to end offscreenCommandBuffer");
    }
}*/

void VulkanApp::createSyncObjects()
{
    waitFences.resize(MAX_FRAMES_IN_FLIGHT);
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (auto& fence : waitFences)
    {
        if (vkCreateFence(device_, &fenceCreateInfo, nullptr, &fence))
        {
            throw std::runtime_error("failed to create fence!");
        }
    }


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

void VulkanApp::updateUniformBuffer(uint32_t currentImage, glm::mat4 modelMatrix) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    DeferredUniformBufferObject ubo = {};

    // update model position
    ubo.modelMatrix = modelMatrix;

    glm::mat4 modelMat(0.1f);
    float scale = 0.1f;
    glm::scale(modelMat, glm::vec3(scale, scale, scale));

    ubo.modelMatrix = modelMat;

    // update camera
    // ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    
    ubo.view = orbitCam->GetViewMat();
    ubo.proj = orbitCam->GetProjMat();
    

    if (debugCam) {
        ubo.deferredProj = glm::ortho(0.0f, 2.0f, 0.0f, 2.0f, -1.0f, 1.0f);
    }
    else {
        ubo.deferredProj = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
    }

    float delta = 1.f;

    ubo.deferredProj = glm::mat4(
        { delta, delta, 0, delta },
        { 0, delta, 0, delta },
        { delta, 0, delta, delta },
        { -delta, -delta, -delta, 0 });

    // vs full screen uniform buffer data update
    void* data;
    vkMapMemory(device_, uniformBuffers.vsFullScreen.deviceMem, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device_, uniformBuffers.vsFullScreen.deviceMem);

    // vs off screen uniform buffer data update
    void* offScreenData;
    vkMapMemory(device_,
        offscreen_.uniformBufferAndContent.uniformBuffer.deviceMemory,
        0, sizeof(ubo), 0, &offScreenData);
    memcpy(offScreenData, &ubo, sizeof(ubo));
    vkUnmapMemory(device_, offscreen_.uniformBufferAndContent.uniformBuffer.deviceMemory);
}



void VulkanApp::draw()
{
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

    // wait for swap chian presentation to finish
    mySubmitInfo.pWaitSemaphores = waitSemaphores;
    mySubmitInfo.pSignalSemaphores = &offscreenSemaphore;
    mySubmitInfo.commandBufferCount = 1;
    mySubmitInfo.pCommandBuffers = &offscreen_.commandBuffer;

    // todo set wait semaphores and signal semaphores
    if (vkQueueSubmit(queue_, 1, &mySubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit queue 1");
    }


    // wait for offsreen
    VkSemaphore waitSemaphores_2[] = { offscreenSemaphore };
    mySubmitInfo.pWaitSemaphores = waitSemaphores_2;
    mySubmitInfo.pSignalSemaphores = &semaphores_.renderComplete;
    mySubmitInfo.pCommandBuffers = &command_buffers_[imageIndex];
    if (vkQueueSubmit(queue_, 1, &mySubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit queue 2");
    }

    VkResult res = queuePresent(queue_, imageIndex, semaphores_.renderComplete);
    vkQueueWaitIdle(queue_);

    
    updateUniformBuffer(imageIndex, glm::mat4(1.f));
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

void VulkanApp::createOffscreenFramebuffers() {

    //// (World space) Positions
    ////createAttachment(
    ////    VK_FORMAT_R16G16B16A16_SFLOAT,
    ////    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    ////    &offScreenFrameBuf.position);

    //// world space pos
    //VkFormat positionFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    //createImage(swapchain_extent_.width, swapchain_extent_.height, positionFormat,
    //    VK_IMAGE_TILING_OPTIMAL,
    //    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //    offScreenFrameBuf.position.image, offScreenFrameBuf.position.deviceMemory);
    //offScreenFrameBuf.position.imageView = createImageView(offScreenFrameBuf.position.image, positionFormat,
    //    VK_IMAGE_ASPECT_COLOR_BIT);

    //// world space normal
    //VkFormat normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    //createImage(swapchain_extent_.width, swapchain_extent_.height, normalFormat,
    //    VK_IMAGE_TILING_OPTIMAL,
    //    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //    offScreenFrameBuf.normal.image, offScreenFrameBuf.normal.deviceMemory);
    //offScreenFrameBuf.normal.imageView = createImageView(offScreenFrameBuf.normal.image, normalFormat,
    //    VK_IMAGE_ASPECT_COLOR_BIT);

    //// Albedo (color)
    //VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    //createImage(swapchain_extent_.width, swapchain_extent_.height, colorFormat,
    //    VK_IMAGE_TILING_OPTIMAL,
    //    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //    offScreenFrameBuf.albedo.image, offScreenFrameBuf.albedo.deviceMemory);
    //offScreenFrameBuf.albedo.imageView = createImageView(offScreenFrameBuf.albedo.image, colorFormat,
    //    VK_IMAGE_ASPECT_COLOR_BIT);

    //// Depth (color)
    //VkFormat depthFormat = findDepthFormat();
    //createImage(swapchain_extent_.width, swapchain_extent_.height, depthFormat,
    //    VK_IMAGE_TILING_OPTIMAL,
    //    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //    offScreenFrameBuf.depth.image, offScreenFrameBuf.depth.deviceMemory);
    //offScreenFrameBuf.depth.imageView = createImageView(offScreenFrameBuf.depth.image, depthFormat,
    //    VK_IMAGE_ASPECT_DEPTH_BIT);

    //// Set up separate renderpass with references to the color and depth attachments
    //std::array<VkAttachmentDescription, 4> attachmentDescs = {};

    //// metallic here // wrong
    //for (uint32_t i = 0; i < 4; ++i)
    //{
    //    attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
    //    attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //    attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //    attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //    attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //    if (i == 3)
    //    {
    //        attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //        attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    //    }
    //    else
    //    {
    //        attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //        attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //    }
    //}

    //attachmentDescs[0].format = positionFormat;
    //attachmentDescs[1].format = normalFormat;
    //attachmentDescs[2].format = colorFormat;
    //attachmentDescs[3].format = depthFormat;

    //std::vector<VkAttachmentReference> colorReferences;
    //colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    //colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    //colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

    //VkAttachmentReference depthReference = {};
    //depthReference.attachment = 3;
    //depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //VkSubpassDescription subpass = {};
    //subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    //subpass.pColorAttachments = colorReferences.data();
    //subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    //subpass.pDepthStencilAttachment = &depthReference;

    //std::array<VkSubpassDependency, 2> dependencies;

    //dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    //dependencies[0].dstSubpass = 0;
    //dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    //dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    //dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //dependencies[1].srcSubpass = 0;
    //dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    //dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    //dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    //dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //VkRenderPassCreateInfo renderPassInfo = {};
    //renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    //renderPassInfo.pAttachments = attachmentDescs.data();
    //renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
    //renderPassInfo.subpassCount = 1;
    //renderPassInfo.pSubpasses = &subpass;
    //renderPassInfo.dependencyCount = 2;
    //renderPassInfo.pDependencies = dependencies.data();

    //if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &offScreenFrameBuf.renderPass) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create render pass for offscreen frame buffer");
    //}

    //std::array<VkImageView, 4> attachments;
    //attachments[0] = offScreenFrameBuf.position.imageView;
    //attachments[1] = offScreenFrameBuf.normal.imageView;
    //attachments[2] = offScreenFrameBuf.albedo.imageView;
    //attachments[3] = offScreenFrameBuf.depth.imageView;

    //VkFramebufferCreateInfo fbufCreateInfo = {};
    //fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    //fbufCreateInfo.pNext = NULL;
    //fbufCreateInfo.renderPass = offScreenFrameBuf.renderPass;
    //fbufCreateInfo.pAttachments = attachments.data();
    //fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    //fbufCreateInfo.width = swapchain_extent_.width;
    //fbufCreateInfo.height = swapchain_extent_.height;
    //fbufCreateInfo.layers = 1;

    //if (vkCreateFramebuffer(device_, &fbufCreateInfo, nullptr, &offScreenFrameBuf.frameBuffer) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create offscreen frame buffer");
    //}

    //VkSamplerCreateInfo samplerCreateInfo{};
    //samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    //samplerCreateInfo.maxAnisotropy = 1.0f;
    //samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    //samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    //samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    //samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    //samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
    //samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
    //samplerCreateInfo.mipLodBias = 0.0f;
    //samplerCreateInfo.maxAnisotropy = 1.0f;
    //samplerCreateInfo.minLod = 0.0f;
    //samplerCreateInfo.maxLod = 1.0f;
    //samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    //if (vkCreateSampler(device_, &samplerCreateInfo, nullptr, &colorSampler) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to create offscreen frame buffer sampler");
    //}
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


    vertices_new.bindingDescriptions.resize(1);
    
    VkVertexInputBindingDescription vInputBindDescription{};
    vInputBindDescription.binding = 0;
    vInputBindDescription.stride = 56;
    vInputBindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertices_new.bindingDescriptions[0] = vInputBindDescription;
    //vertices_new.bindingDescriptions = vInputBindDescription;
    vertices_new.attributeDescriptions.resize(5);

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

    //vertices_new.attributeDescriptions = { posAttrDesc, uvAttrDesc,
    //    colorAttrDesc, normalAttrDesc, tanAttrDesc };

    vertices_new.attributeDescriptions[0] = posAttrDesc;
    vertices_new.attributeDescriptions[1] = uvAttrDesc;
    vertices_new.attributeDescriptions[2] = colorAttrDesc;
    vertices_new.attributeDescriptions[3] = normalAttrDesc;
    vertices_new.attributeDescriptions[4] = tanAttrDesc;


    VkPipelineVertexInputStateCreateInfo inputStateInfo{};
    inputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertices_new.bindingDescriptions.size());
    inputStateInfo.pVertexBindingDescriptions = vertices_new.bindingDescriptions.data();

   // inputStateInfo.pVertexBindingDescriptions

    inputStateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices_new.attributeDescriptions.size());
    inputStateInfo.pVertexAttributeDescriptions = vertices_new.attributeDescriptions.data();
    vertices_new.inputState = inputStateInfo;
    //uint32_t binding,
    //    uint32_t stride,
    //    VkVertexInputRate inputRate)
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

void VulkanApp::createScene() {
    ModelVertexIndexTextureBuffer plane;
    plane.paths.albedoTexturePath = "../../textures/uv_debug.png";
    plane.paths.normalTexturePath = "../../textures/normal_map_debug.png";
    models.push_back(plane);

    ModelVertexIndexTextureBuffer house;
    house.paths.modelPath = "../../models/rock.obj";
    house.paths.albedoTexturePath = "../../textures/rock_low_Base_Color.png";
    house.paths.normalTexturePath = "../../textures/rock_low_Normal_DirectX.png";
    models.push_back(house);
}

// ray tracing

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

Sphere VulkanApp::newSphere(glm::vec3 pos, float radius, glm::vec3 diffuse, float specular)
{
    Sphere sphere;
    sphere.id = currentId++;
    sphere.pos = pos;
    sphere.radius = radius;
    sphere.diffuse = diffuse;
    sphere.specular = specular;
    return sphere;
}

Plane VulkanApp::newPlane(glm::vec3 normal, float distance, glm::vec3 diffuse, float specular)
{
    Plane plane;
    plane.id = currentId++;
    plane.normal = normal;
    plane.distance = distance;
    plane.diffuse = diffuse;
    plane.specular = specular;
    return plane;
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


    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &graphics.rt_descriptorSetLayout;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, nullptr, &graphics.rt_raytracePipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create rt_raytracePipelineLayout for rt!");
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
    computePipelineCreateInfo.stage = loadShader("C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/raytracing.comp.spv",
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


void VulkanApp::rt_createRaytraceDisplayCommandBuffer() {
    rt_drawCommandBuffer.resize(swapchain_framebuffers_.size());

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
        // we do want to use deferredRenderPass
        renderPassBeginInfo.renderPass = deferred_renderpass_;
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
    mySubmitInfo.pSignalSemaphores = &offscreenSemaphore;
    mySubmitInfo.commandBufferCount = 1;
    mySubmitInfo.pCommandBuffers = &compute.rt_computeCmdBuffer;

    // Command buffer to be sumitted to the queue
    if (vkQueueSubmit(queue_, 1, &mySubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("can't submit compute.rt_computeCmdBuffer in graphicsQueue");
    }

    // wait for offsreen
    VkSemaphore waitSemaphores_2[] = { offscreenSemaphore };
    mySubmitInfo.pWaitSemaphores = waitSemaphores_2;
    mySubmitInfo.pSignalSemaphores = &semaphores_.renderComplete;
    mySubmitInfo.pCommandBuffers = &rt_drawCommandBuffer[imageIndex];
    if (vkQueueSubmit(queue_, 1, &mySubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit rt_drawCommandBuffer in graphicsQueue");
    }

    VkResult res = queuePresent(queue_, imageIndex, semaphores_.renderComplete);
    vkQueueWaitIdle(queue_);
    rt_updateUniformBuffer();
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

void VulkanApp::prepareOffscreen() {
    createOffscreenDescriptorSetLayout();
    createOffscreenUniformBuffer();
    createOffscreenPipelineLayout();
    createOffscreenRenderPass();
    createOffscreenFrameBuffer();
    createOffscreenPipeline();
    createOffscreenCommandBuffer();
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
            VK_SHADER_STAGE_FRAGMENT_BIT)
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
    // HERE 0
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
        "C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/mrt.vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(
        "C:/Users/Zichuan/Documents/Vulkan_Hybrid_PBR/shaders/mrt.frag.spv",
        VK_SHADER_STAGE_FRAGMENT_BIT);

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
    pipelineCreateInfo.pVertexInputState = &vertices_new.inputState;

    std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates = {
            blendAttachmentState,
            blendAttachmentState,
            blendAttachmentState,
            blendAttachmentState
    };

    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments = blendAttachmentStates.data();
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
    std::array<VkAttachmentDescription, 5> attachmentDescs = {};

    for (uint32_t i = 0; i < 5; ++i)
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

void VulkanApp::createOffscreenCommandBuffer() {
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

    if (vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &offscreenSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("failed to create offscreenSemaphore");
    }

    VkCommandBufferBeginInfo cmdBufInfo{};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    std::array<VkClearValue, 5> clearValues;
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
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

    if (vkBeginCommandBuffer(offscreen_.commandBuffer, &cmdBufInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin offscreen_.commandBuffer");
    }

    vkCmdBeginRenderPass(offscreen_.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width = swapchain_extent_.width;
    viewport.height = swapchain_extent_.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(offscreen_.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent.width = swapchain_extent_.width;
    scissor.extent.height = swapchain_extent_.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(offscreen_.commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(
        offscreen_.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        offscreen_.pipeline);

    VkDeviceSize offsets[1] = { 0 };

    // Draw scene object 0
    vkCmdBindDescriptorSets(offscreen_.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS, offscreen_.pipelineLayout, 0, 1,
        &scene_objects_[0].descriptorSet, 0, NULL);

    vkCmdBindVertexBuffers(offscreen_.commandBuffer, 0, 1,
        &scene_objects_[0].vertexBuffer.buffer, offsets);

    vkCmdBindIndexBuffer(offscreen_.commandBuffer,
        scene_objects_[0].indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(
        offscreen_.commandBuffer,
        static_cast<uint32_t>(indices_.size()),
        1, 0, 0, 0);

    vkCmdEndRenderPass(offscreen_.commandBuffer);

    if (vkEndCommandBuffer(offscreen_.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to end offscreenCommandBuffer");
    }
}

// scene objects =================================================
void VulkanApp::prepareSceneObjectsData() {
    AppSceneObject rock1{};
    rock1.meshPath = "../../models/rock.obj";
    rock1.albedo.path = "../../textures/rock_low_Base_Color.png";
    rock1.normal.path = "../../textures/rock_low_Normal_DirectX.png";
    rock1.mrao.path = "../../textures/rock_low_Normal_DirectX.png";
    scene_objects_.push_back(rock1);

    // the following 2 functions must be called before scene object loading
    // scene object descriptor set requires offscreen uniform buffer and
    // descriptor set layout
    // createOffscreenDescriptorSetLayout()
    // createOffscreenUniformBuffer()

    for (auto scene_object : scene_objects_) {
        loadSceneObjectMesh(scene_object);
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
    for (auto scene_object : scene_objects_) {
        // allocate descriptor
        createSceneObjectDescriptorSet(scene_object);
    }
}

void VulkanApp::loadAllSceneObjectTexture(AppSceneObject& scene_object) {

    // albedo texture 
    int albedo_texWidth, albedo_texHeight, albedo_texChannels;
    stbi_uc* albedo_pixels = stbi_load(scene_object.albedo.path.c_str(),
        &albedo_texWidth, &albedo_texHeight,
        &albedo_texChannels, STBI_rgb_alpha);
    VkDeviceSize albedo_imageSize = albedo_texWidth * albedo_texHeight * 4;

    if (!albedo_pixels) {
        throw std::runtime_error("failed to load abledo texture image!");
    }

    // -------------- albedo --------------
    VkBuffer albedo_stagingBuffer;
    VkDeviceMemory albedo_stagingBufferMemory;

    createBuffer(albedo_imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        albedo_stagingBuffer, albedo_stagingBufferMemory);

    void* data;
    vkMapMemory(device_, albedo_stagingBufferMemory, 0, albedo_imageSize, 0, &data);
    memcpy(data, albedo_pixels, static_cast<size_t>(albedo_imageSize));
    vkUnmapMemory(device_, albedo_stagingBufferMemory);

    stbi_image_free(albedo_pixels);

    createImage(albedo_texWidth, albedo_texHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        scene_object.albedo.texture.image,
        scene_object.albedo.texture.deviceMemory);

    // transition layout
    transitionImageLayout(scene_object.albedo.texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(albedo_stagingBuffer, scene_object.albedo.texture.image,
        static_cast<uint32_t>(albedo_texWidth),
        static_cast<uint32_t>(albedo_texHeight));
    transitionImageLayout(scene_object.albedo.texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device_, albedo_stagingBuffer, nullptr);
    vkFreeMemory(device_, albedo_stagingBufferMemory, nullptr);

    // -------------- normal --------------
    int normal_texWidth, normal_texHeight, normal_texChannels;
    stbi_uc* normal_pixels = stbi_load(scene_object.normal.path.c_str(),
        &normal_texWidth, &normal_texHeight, &normal_texChannels,
        STBI_rgb_alpha);
    VkDeviceSize normal_imageSize = normal_texWidth * normal_texHeight * 4;

    if (!normal_pixels) {
        throw std::runtime_error("failed to load abledo texture image!");
    }

    VkBuffer normal_stagingBuffer;
    VkDeviceMemory normal_stagingBufferMemory;

    createBuffer(normal_imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        normal_stagingBuffer, normal_stagingBufferMemory);

    void* normalData;

    vkMapMemory(device_, normal_stagingBufferMemory, 0, normal_imageSize, 0, &normalData);
    memcpy(normalData, normal_pixels, static_cast<size_t>(normal_imageSize));
    vkUnmapMemory(device_, normal_stagingBufferMemory);

    stbi_image_free(normal_pixels);

    createImage(normal_texWidth, normal_texHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        scene_object.normal.texture.image,
        scene_object.normal.texture.deviceMemory);

    transitionImageLayout(scene_object.normal.texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(normal_stagingBuffer, scene_object.normal.texture.image,
        static_cast<uint32_t>(normal_texWidth),
        static_cast<uint32_t>(normal_texHeight));
    transitionImageLayout(scene_object.normal.texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkDestroyBuffer(device_, normal_stagingBuffer, nullptr);
    vkFreeMemory(device_, normal_stagingBufferMemory, nullptr);

    // -------------- mrao --------------
    int mrao_texWidth, mrao_texHeight, mrao_texChannels;
    stbi_uc* mrao_pixels = stbi_load(scene_object.mrao.path.c_str(),
        &mrao_texWidth, &mrao_texHeight, &mrao_texChannels,
        STBI_rgb_alpha);
    VkDeviceSize mrao_imageSize = mrao_texWidth * mrao_texHeight * 4;

    if (!mrao_pixels) {
        throw std::runtime_error("failed to load abledo texture image!");
    }

    VkBuffer mrao_stagingBuffer;
    VkDeviceMemory mrao_stagingBufferMemory;

    createBuffer(mrao_imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        mrao_stagingBuffer, mrao_stagingBufferMemory);

    void* mraoData;

    vkMapMemory(device_, mrao_stagingBufferMemory, 0, mrao_imageSize, 0, &mraoData);
    memcpy(mraoData, mrao_pixels, static_cast<size_t>(mrao_imageSize));
    vkUnmapMemory(device_, mrao_stagingBufferMemory);

    stbi_image_free(mrao_pixels);

    createImage(mrao_texWidth, mrao_texHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        scene_object.mrao.texture.image,
        scene_object.mrao.texture.deviceMemory);

    transitionImageLayout(scene_object.mrao.texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(mrao_stagingBuffer, scene_object.mrao.texture.image,
        static_cast<uint32_t>(mrao_texWidth),
        static_cast<uint32_t>(mrao_texHeight));
    transitionImageLayout(scene_object.mrao.texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkDestroyBuffer(device_, mrao_stagingBuffer, nullptr);
    vkFreeMemory(device_, mrao_stagingBufferMemory, nullptr);
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
            1)
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

// deferred =================================================
void VulkanApp::prepareDeferred() {
    prepareQuadVertexAndIndexBuffer();
    createDeferredUniformBuffer();
    createDeferredDescriptorSetLayout();
    createDeferredDescriptorSet();
    createDeferredPipelineLayout();
    createDeferredRenderPass();
    createDeferredPipeline();
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

void VulkanApp::createDeferredCommandBuffer() {
    deferred_command_buffers_.resize(swapchain_framebuffers_.size());

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

        VkBuffer vertexBuffers[] = { quadVertexBuffer };
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

        if (vkEndCommandBuffer(command_buffers_[i]) != VK_SUCCESS) {
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
    mySubmitInfo.pSignalSemaphores = &offscreenSemaphore;
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
    VkSemaphore waitSemaphores_2[] = { offscreenSemaphore };
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




