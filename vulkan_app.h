#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <glm/gtx/hash.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <unordered_map>
#include "camera.h"

const int WIDTH = 800;
const int HEIGHT = 600;

//const std::string MODEL_PATH = "models/chalet.obj";
//const std::string ALBEDO_TEXTURE_PATH = "textures/chalet_old.jpg";
//const std::string NORMALMAP_TEXTURE_PATH = "textures/chalet_old.jpg";

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


struct DeferredUniformBufferObject {
    glm::mat4 modelMatrix;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 deferredProj;
};

struct RTUniformBufferObject {							// Compute shader uniform block object
    glm::vec3 lightPos = glm::vec3(0.f, 0.f, 0.f);
    float aspectRatio = 1.333f;						// Aspect ratio of the viewport
    glm::vec4 fogColor = glm::vec4(0.0f);
    struct {
        glm::vec3 pos = glm::vec3(0.0f, 0.0f, 4.0f);
        glm::vec3 lookat = glm::vec3(0.0f, 0.5f, 0.0f);
        float fov = 10.0f;
    } camera;
};

struct MyTexture
{
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
};


struct ModelVertexIndexTextureBuffer {
    uint32_t vertexCount;
    uint32_t indexCount;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMem;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMem;

    MyTexture albedoTexture;
    MyTexture normalMapTexture;
    struct Paths{
        std::string modelPath;
        std::string albedoTexturePath;
        std::string normalTexturePath;
    } paths;

    VkDescriptorSet descriptorSet;
};

struct MyUniformBuffer {
    VkBuffer buffer;
    VkDeviceMemory deviceMem;
    VkDescriptorBufferInfo desBuffInfo;
    //VkBufferUsageFlags bufferUsageFlags;
    //VkMemoryPropertyFlags memProperyFlags;
};

struct Sphere {									// Shader uses std140 layout (so we only use vec4 instead of vec3)
    glm::vec3 pos;
    float radius;
    glm::vec3 diffuse;
    float specular;
    uint32_t id;								// Id used to identify sphere for raytracing
    glm::ivec3 _pad;
};

// SSBO plane declaration
struct Plane {
    glm::vec3 normal;
    float distance;
    glm::vec3 diffuse;
    float specular;
    uint32_t id;
    glm::ivec3 _pad;
};

class VulkanApp {
public:
    VulkanApp();
    void run();

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT callback;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    // VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass deferredRenderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    // TODO
    //VkPipelineLayout pipelineLayout;
    //VkPipeline graphicsPipeline;

    VkCommandPool commandPool;

    struct FrameBufferAttachment {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    };

    struct FrameBuffer {
        VkFramebuffer frameBuffer;
        FrameBufferAttachment position, normal, albedo;
        FrameBufferAttachment depth;
        VkRenderPass renderPass;
    } offScreenFrameBuf;

   //buffer: vkBuffer vkdevicememory vkdescriptorbufferInfo vkbufferusageFlags vkmemPropertyFlags;

    struct {
        VkPipelineVertexInputStateCreateInfo inputState;
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    } vertices_new;


    FrameBufferAttachment depthAttachment;

    const std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    VkDescriptorPool descriptorPool;

    std::vector<VkCommandBuffer> commandBuffers;

    bool framebufferResized = false;

    // MEMBER VAR

    void initWindow();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void initVulkan();

    void mainLoop();

    void cleanupSwapChain();

    void cleanup();

    void recreateSwapChain();

    void createInstance();

    void setupDebugCallback();

    void createSurface();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapChain();

    void createImageViews();

    void createRenderPass();

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createDepthResources();

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

    bool hasStencilComponent(VkFormat format);

    void createTextureImage(ModelVertexIndexTextureBuffer& model);

    void createTextureImageView(ModelVertexIndexTextureBuffer& model);

    void createTextureSampler(ModelVertexIndexTextureBuffer& model);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void loadModel();

    void createHardCodeModelVertexBuffer();

    void createHardCodeModelIndexBuffer();

    void createUniformBuffers();

    void createDescriptorPool();

    void createDescriptorSets();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createFinalRenderCommandBuffers();
    void createSyncObjects();
    void updateUniformBuffer(uint32_t currentImage, glm::mat4 modelMatrix);

    VkShaderModule createShaderModule(const std::vector<char>& code);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    std::vector<const char*> getRequiredExtensions();

    bool checkValidationLayerSupport();

    static std::vector<char> readFile(const std::string& filename);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);


    void createPipelineCache();

    VkPipelineCache pipelineCache;

    void createOffscreenFramebuffers();

    VkSampler colorSampler;

    // mouse & cam
    void initCam();
    static void mouseDownCallback(GLFWwindow* window, int button, int action, int mods);
    static void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition);

    // debug view
    bool debugCam = false;

    // descriptor set
    // union buffer
    struct {
        MyUniformBuffer vsFullScreen;
        MyUniformBuffer vsOffScreen;
        MyUniformBuffer rt_compute;
    } uniformBuffers;

    VkPipelineLayout deferredPipelineLayout;
    VkPipelineLayout offscreenPipelineLayout;
    VkPipeline deferredPipeline;
    VkPipeline offscreenPipeline;

    VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

    std::vector<VkShaderModule> shaderModules;

    VkDescriptorSet quadDescriptorSet;
    VkCommandBuffer offscreenCommandBuffer = VK_NULL_HANDLE;

    void createOffscreenCommandBuffer();


    void draw();

    void createQuadVertexBuffer();
    void createQuadIndexBuffer();
    void prepareQuad();

    VkBuffer quadVertexBuffer;
    VkDeviceMemory quadVertexBufferMemory;

    VkBuffer quadIndexBuffer;
    VkDeviceMemory quadIndexBufferMemory;

    void setupVertexDescriptions();

    VkSubmitInfo mySubmitInfo;

    struct {
        // Swap chain image presentation
        VkSemaphore presentComplete;
        // Command buffer submission and execution
        VkSemaphore renderComplete;
    } semaphores;


    VkSemaphore offscreenSemaphore = VK_NULL_HANDLE;
    std::vector<VkFence> waitFences;
    void initSemAndSubmitInfo();

    VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);


    std::vector<ModelVertexIndexTextureBuffer> models;

    void loadArbitaryModelAndBuffers(ModelVertexIndexTextureBuffer& model_struct );

    void createScene();

    // ray tracing

    void rt_prepareStorageBuffers();

    Sphere newSphere(glm::vec3 pos, float radius, glm::vec3 diffuse, float specular);

    Plane newPlane(glm::vec3 normal, float distance, glm::vec3 diffuse, float specular);

    uint32_t currentId = 0;
    
    void rt_prepareTextureTarget(MyTexture& tex, VkFormat format, uint32_t width = WIDTH, uint32_t height = HEIGHT);

    void rt_graphics_setupDescriptorSetLayout();

    MyTexture rt_result;

    // todo: ray tracing part share the same queue with whom???????
    struct {
      //  VkDescriptorSet descriptorSetPreCompute;	// Raytraced image display shader bindings before compute shader image manipulation
      //  VkDescriptorSet descriptorSet;				// Raytraced image display shader bindings after compute shader image manipulation
        VkDescriptorSetLayout rt_descriptorSetLayout;
        VkDescriptorSet rt_descriptorSet;
        VkPipelineLayout rt_raytracePipelineLayout;
        VkPipeline rt_raytracePipeline;
        VkRenderPass rt_rayTraceRenderPass;
    } graphics;


    //struct {
    //    MyUniformBuffer vsFullScreen;
    //    MyUniformBuffer vsOffScreen;
    //    MyUniformBuffer rt_compute;
    //} uniformBuffers;


    void rt_graphics_setupDescriptorSet();
    void rt_prepareCompute();
    struct {
        struct rayTracingSceneObjectBuffer
        {
            VkBuffer buffer;
            VkDeviceMemory deviceMem;
        }myPlaneBuffer, mySphereBuffer;

        VkDescriptorSetLayout rt_computeDescriptorSetLayout;
        VkDescriptorSet rt_computeDescriptorSet;
        VkPipelineLayout rt_computePipelineLayout;
;
        VkPipeline rt_computePipine;
        VkQueue rt_computeQueue;
        VkFence rt_fence;
        VkCommandBuffer rt_computeCmdBuffer = VK_NULL_HANDLE;

    } compute;

    void rt_createRaytraceDisplayCommandBuffer();
    std::vector<VkCommandBuffer> rt_drawCommandBuffer;

    void rt_draw();

    void createGraphicsPipeline_old();

    void rt_createComputeCommandBuffer();

    void rt_updateUniformBuffer();

    RTUniformBufferObject rt_ubo;
};
