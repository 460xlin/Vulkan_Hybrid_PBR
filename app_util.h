#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <glm/glm.hpp>

struct AppUniformBuffer {
    VkBuffer buffer;
    VkDeviceMemory deviceMemory;
    VkDescriptorBufferInfo descriptorBufferInfo;
};

struct AppTexture
{
    VkImage image;
    VkDeviceMemory deviceMemory;
    VkImageView imageView;
    VkSampler sampler;
    VkDescriptorImageInfo descriptorImageInfo;
};

struct AppFramebufferAttachment {
    VkImage image;
    VkDeviceMemory deviceMemory;
    VkImageView imageView;
};

struct AppTextureInfo {
    std::string path;
    AppTexture texture;
};

struct AppSceneObjectUniformBufferConent {
    glm::mat4 modelMatrix;
};




struct AppSceneObject {
    
    std::string meshPath;
    uint32_t vertexCount;
    uint32_t indexCount;

    struct {
        VkBuffer buffer;
        VkDeviceMemory deviceMemory;
    } vertexBuffer, indexBuffer;

    AppTextureInfo albedo, normal, mrao;

    VkDescriptorSet descriptorSet;

    struct {
        AppUniformBuffer uniformBuffer;
        AppSceneObjectUniformBufferConent content;
    } uniformBufferAndContent; 
};

struct RT_AppSceneObject {

	std::string meshPath;
	uint32_t vertexCount;
	uint32_t indexCount;
	uint32_t triangleCount;

	VkBuffer buffer;
	VkDeviceMemory deviceMem;
	VkDescriptorBufferInfo buffInfo;

	VkDescriptorSet descriptorSet;
};

struct AppDeferredUniformBufferContent {
    glm::vec3 eyePos;
    glm::vec3 lightPos;
    glm::mat4 modelView;
};

struct AppDeferredPipelineAssets {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    struct {
        AppTextureInfo brdfLUT;
    } pbrTextures;
    struct {
        AppUniformBuffer uniformBuffer;
        AppDeferredUniformBufferContent content;
    } uniformBufferAndContent;
    // output framebuf is swapchain framebuf
    // if do post processing
    // add a framebuffer to store image output
};

struct AppOffscreenUniformBufferContent {
    glm::mat4 projMatrix;
    glm::mat4 viewMatrix;
};

struct AppOffscreenPipelineAssets {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkCommandBuffer commandBuffer;

    struct {
        AppOffscreenUniformBufferContent content;
        AppUniformBuffer uniformBuffer;
    } uniformBufferAndContent;

    struct {
        VkFramebuffer frameBuffer;
        AppTexture position, normal, color, mrao;
        AppTexture depth;
        VkSampler sampler;
    } frameBufferAssets;
};

struct AppSkyBoxUniformBufferContent {
    glm::mat4 projMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    float lodBias = 0.0f;
};


struct AppSkyboxSceneObject {
    AppSceneObject mesh;
    struct {
        AppTextureInfo textureInfo;
        uint32_t width;
        uint32_t height;
        uint32_t mipLevels;
    } cubemap;
};


struct AppSkyboxPipelineAssets {
    struct {
        AppSkyBoxUniformBufferContent content;
        AppUniformBuffer uniformBuffer;
    } uniformBufferAndContent;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    // only one descriptorSet, in cube's descriptorSet
    // VkDescriptorSet descriptorSet;
    VkCommandBuffer commandBuffer;
    AppSkyboxSceneObject skyBoxCube;
};



namespace apputil {
    VkDescriptorSetLayoutBinding createDescriptorSetLayoutBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        uint32_t descriptorCount,
        VkShaderStageFlags stageFlags);

    VkPipelineInputAssemblyStateCreateInfo createInputAssemblyStateCreateInfo(
        VkPipelineInputAssemblyStateCreateFlags flags,
        VkPrimitiveTopology topology,
        VkBool32 primitiveRestartEnable);

    VkWriteDescriptorSet createBufferWriteDescriptorSet(
        VkDescriptorSet dstSet,
        VkDescriptorType descriptorType,
        uint32_t dstBinding,
        const VkDescriptorBufferInfo* pBufferInfo,
        uint32_t descriptorCount);

    VkWriteDescriptorSet createImageWriteDescriptorSet(
        VkDescriptorSet dstSet,
        VkDescriptorType descriptorType,
        uint32_t dstBinding,
        const VkDescriptorImageInfo* pBuffepImageInforInfo,
        uint32_t descriptorCount);

    VkCommandBufferBeginInfo cmdBufferBegin(VkCommandBufferUsageFlags flags);

    void printVec3(const glm::vec3& vector);
}


struct Vertex {
	float pos[3];
	float uv[2];
	float col[3];
	float normal[3];
	float tangent[3];
};


struct BoundingBox
{
	glm::vec3 maxB;
	glm::vec3 minB;
};

struct Triangle
{
	int triidx;
	Vertex Triverts[3];
	glm::vec3 Trinormal;
	BoundingBox bbx;
};