#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

struct AppTexture
{
    VkImage image;
    VkDeviceMemory deviceMemory;
    VkImageView imageView;
    VkSampler sampler;
    VkDescriptorImageInfo descriptorImageInfo;
};

struct AppUniformBuffer {
    VkBuffer buffer;
    VkDeviceMemory deviceMemory;
    VkDescriptorBufferInfo descriptorBufferInfo;
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
    AppUniformBuffer modelMatrixBuffer;
};

struct AppDeferredPipelineAssets {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkCommandBuffer commandBuffer;
    AppUniformBuffer lightUniformBuffer;
    // output framebuf is swapchain framebuf
    // if do post processing
    // add a framebuffer to store image output
};

struct AppOffscreenPipelineAssets {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkCommandBuffer commandBuffer;
    AppUniformBuffer cameraUniformBuffer;

    struct {
        VkFramebuffer frameBuffer;
        AppFramebufferAttachment position, normal, albedo;
        AppFramebufferAttachment depth;
    } frameBufferAssets;
};

//
//    VkDescriptorSetLayoutBinding createDescriptorSetLayoutBinding(uint32_t binding,
//        VkDescriptorType descriptorType,
//        uint32_t descriptorCount,
//        VkShaderStageFlags stageFlags) {
//        VkDescriptorSetLayoutBinding layoutBinding = {};
//        layoutBinding.binding = binding;
//        layoutBinding.descriptorType = descriptorType;
//        layoutBinding.descriptorCount = descriptorCount;
//        layoutBinding.stageFlags = stageFlags;
//        layoutBinding.pImmutableSamplers = nullptr;
//        return layoutBinding;
//    }
//}
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
}


