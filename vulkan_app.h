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
#include "app_util.h"

const int WIDTH = 800;
const int HEIGHT = 600;

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
    GLFWwindow* window_;

    VkInstance instance_;
    VkDebugUtilsMessengerEXT callback_;
    VkSurfaceKHR surface_;

    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_;

    VkQueue queue_;

    VkSwapchainKHR swapchain_;
    std::vector<VkImage> swapchain_images_;
    VkFormat swapchain_imageformat_;
    VkExtent2D swapchain_extent_;
    std::vector<VkImageView> swapchain_imageviews_;
    std::vector<VkFramebuffer> swapchain_framebuffers_;

    VkRenderPass deferred_renderpass_;
    VkDescriptorSetLayout descriptor_set_layout_;

    VkCommandPool command_pool_;

    struct {
        VkPipelineVertexInputStateCreateInfo inputState;
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    } vertex_input_info;

    AppFramebufferAttachment depth_attachment_;

    VkDescriptorPool descriptor_pool_;

    bool framebufferResized = false;

    // MEMBER VAR

    void initWindow();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void initVulkan();

    void mainLoop();

    void cleanup();

    void createInstance();

    void setupDebugCallback();

    void createSurface();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapChain();

    void createSwapChainImageViews();

    void createCommandPool();

    void createDepthResources();

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

    bool hasStencilComponent(VkFormat format);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


    void createDescriptorPool();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

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

    VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

    std::vector<VkShaderModule> shaderModules;

    void setupVertexDescriptions();

    std::vector<VkFence> waitFences;
    void initSemAndSubmitInfo();

    VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);


	// ray tracing =================================================
	void rt_createSema();
	void rt_createUniformBuffers();
    void rt_prepareStorageBuffers();
	void rt_prepareObjFileBuffer();
	void rt_prepareTextureTarget(MyTexture& tex, VkFormat format, uint32_t width = WIDTH, uint32_t height = HEIGHT);
	void rt_graphics_setupDescriptorSetLayout();
	void rt_graphics_setupDescriptorSet();
	void rt_createPipelineLayout();
	void rt_createRenderpass();
	void rt_creatFramebuffer(); // use swap chains' framebuffers
	void rt_createPipeline();
	//void rt_setupDescriptorPool(); // all pipeline use the same pool
	void rt_prepareCompute();
	void rt_createComputeCommandBuffer();
	void rt_createRaytraceDisplayCommandBuffer();
	void rt_draw();

    Sphere newSphere(glm::vec3 pos, float radius, glm::vec3 diffuse, float specular);
    Plane newPlane(glm::vec3 normal, float distance, glm::vec3 diffuse, float specular);
	void rt_updateUniformBuffer();
	void rt_loadObj(RT_AppSceneObject& object_struct);

	uint32_t rt_currentId = 0;
	MyTexture rt_result;
	RTUniformBufferObject rt_ubo;
	std::vector<VkCommandBuffer> rt_drawCommandBuffer;
	VkSemaphore rt_sema;


    // todo: ray tracing part share the same queue with whom???????
    
		// union buffer
	struct {
		MyUniformBuffer rt_compute;
	} uniformBuffers;
	
	struct {
      //  VkDescriptorSet descriptorSetPreCompute;	// Raytraced image display shader bindings before compute shader image manipulation
      //  VkDescriptorSet descriptorSet;				// Raytraced image display shader bindings after compute shader image manipulation
        VkDescriptorSetLayout rt_descriptorSetLayout;
        VkDescriptorSet rt_descriptorSet;
        VkPipelineLayout rt_raytracePipelineLayout;
        VkPipeline rt_raytracePipeline;
        VkRenderPass rt_rayTraceRenderPass;
    } graphics;

    struct {
        struct rayTracingSceneObjectBuffer
        {
            VkBuffer buffer;
            VkDeviceMemory deviceMem;
        }myPlaneBuffer, mySphereBuffer;

		RT_AppSceneObject rt_scene_obj;

        VkDescriptorSetLayout rt_computeDescriptorSetLayout;
        VkDescriptorSet rt_computeDescriptorSet;
        VkPipelineLayout rt_computePipelineLayout;
;
        VkPipeline rt_computePipine;
        VkQueue rt_computeQueue;
        VkFence rt_fence;
        VkCommandBuffer rt_computeCmdBuffer = VK_NULL_HANDLE;

    } compute;

    // offscreen =================================================
    AppOffscreenPipelineAssets offscreen_;
    VkSemaphore offscreen_semaphore_ = VK_NULL_HANDLE;
    void prepareOffscreen();
    void prepareOffscreenCommandBuffer();
    void createOffscreenUniformBuffer();
    // this one is called before loading model
    void createOffscreenDescriptorSetLayout();
    void createOffscreenPipelineLayout();
    void createOffscreenRenderPass();
    void createOffscreenFrameBuffer();
    void createOffscreenPipeline();

    // scene loading and prepare assets =================================================
    std::vector<AppSceneObject> scene_objects_;
    void prepareSceneObjectsData();
    void prepareSceneObjectsDescriptor();
    void loadSingleSceneObjectMesh(AppSceneObject& scene_object);
    void loadSingleSceneObjectTexture(AppTextureInfo& texture);
    void createModelMatrixUniformBuffer(AppSceneObject& scene_object);
    void createSceneObjectDescriptorSet(AppSceneObject& scene_object);

    // skybox =================================================
    AppSkyboxPipelineAssets skybox_;
    VkPhysicalDeviceFeatures device_features_;
    VkPhysicalDeviceFeatures enabled_device_features_{};
    VkPhysicalDeviceMemoryProperties device_memory_properties_;
    void prepareSkybox();
    void prepareSkyboxTexture();
    void loadSkyboxMesh();
    void createSkyboxUniformBuffer();
    void createSkyboxDescriptorSetLayout();
    void createSkyboxDescriptorSet();
    void createSkyboxPipelineLayout();
    void createSkyboxPipeline();
    // helper
    void getEnabledFeatures();
    uint32_t skybox_getMemoryType(uint32_t typeBits,
        VkMemoryPropertyFlags properties,
        VkBool32 *memTypeFound = nullptr);
    // TODO: merge this in the original one
    void skybox_transitionLayout(
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask
    );

    // deferred =================================================
    AppDeferredPipelineAssets deferred_;
    std::vector<VkCommandBuffer> deferred_command_buffers_;
    VkBuffer quadVertexBuffer;
    VkDeviceMemory quadVertexBufferMemory;
    VkBuffer quadIndexBuffer;
    VkDeviceMemory quadIndexBufferMemory;
    // TODO: put quad in here after all fo deferred
    // is done
    void prepareDeferred();
    void prepareQuadVertexAndIndexBuffer();
    void createDeferredUniformBuffer();
    void createDeferredPBRTextures();
    void createDeferredDescriptorSetLayout();
    void createDeferredDescriptorSet();
    void createDeferredPipelineLayout();
    void createDeferredRenderPass();
    void createSwapChainFramebuffers();
    void createDeferredPipeline();
    void createDeferredCommandBuffer();
    // helper
    void createQuadVertexBuffer();
    void createQuadIndexBuffer();

    // mouse & cam & input =================================================
    void initCam();
    static void mouseDownCallback(GLFWwindow* window, int button, int action, int mods);
    static void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition);
    static void keyDownCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // general =================================================
    void draw_new();
    void updateUniformBuffers();
    // helper
    void uniformBufferCpy(VkDeviceMemory& device_memory, void* ubo_ptr,
        size_t size);
    glm::mat4 getSkyboxModelMat();
    VkSubmitInfo mySubmitInfo;

    struct {
        // Swap chain image presentation
        VkSemaphore presentComplete;
        // Command buffer submission and execution
        VkSemaphore renderComplete;
    } semaphores_;

	// tryout =================================================
	void createOffscreenForSkyboxAndModel();
};


