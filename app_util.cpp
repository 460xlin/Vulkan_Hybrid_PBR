#include "app_util.h"
#include <iostream>

namespace apputil {
    VkDescriptorSetLayoutBinding createDescriptorSetLayoutBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        uint32_t descriptorCount,
        VkShaderStageFlags stageFlags) {

        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = descriptorCount;
        layoutBinding.stageFlags = stageFlags;
        layoutBinding.pImmutableSamplers = nullptr;

        return layoutBinding;
    }

    VkPipelineInputAssemblyStateCreateInfo createInputAssemblyStateCreateInfo(
        VkPipelineInputAssemblyStateCreateFlags flags,
        VkPrimitiveTopology topology,
        VkBool32 primitiveRestartEnable
    ) {
        VkPipelineInputAssemblyStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        info.flags = flags;
        info.topology = topology;
        info.primitiveRestartEnable = primitiveRestartEnable;
        return info;
    }

    VkPipelineRasterizationStateCreateInfo createPipelineRasterizationStateCreateInfo(
        VkBool32 depthClampEnable,
        VkPolygonMode polygonMode,
        VkCullModeFlags cullMode,
        VkFrontFace frontFace,
        VkPipelineRasterizationStateCreateFlags flags,
        float lineWidth
    ) {
        VkPipelineRasterizationStateCreateInfo rasterizationState{};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.cullMode = VK_CULL_MODE_NONE;
        rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationState.flags = 0;
        rasterizationState.depthClampEnable = VK_FALSE;
        rasterizationState.lineWidth = 1.0f;
        return rasterizationState;
    }

    VkWriteDescriptorSet createBufferWriteDescriptorSet(
        VkDescriptorSet dstSet,
        VkDescriptorType descriptorType,
        uint32_t dstBinding,
        const VkDescriptorBufferInfo* pBufferInfo,
        uint32_t descriptorCount
    ) {
        VkWriteDescriptorSet writeDescSet{};
        writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescSet.dstSet = dstSet;
        writeDescSet.descriptorType = descriptorType;
        writeDescSet.dstBinding = dstBinding;
        writeDescSet.pBufferInfo = pBufferInfo;
        writeDescSet.descriptorCount = descriptorCount;
        return writeDescSet;
    }

    VkWriteDescriptorSet createImageWriteDescriptorSet(
        VkDescriptorSet dstSet,
        VkDescriptorType descriptorType,
        uint32_t dstBinding,
        const VkDescriptorImageInfo* pBuffepImageInforInfo,
        uint32_t descriptorCount
    ) {
        VkWriteDescriptorSet writeDescSet{};
        writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescSet.dstSet = dstSet;
        writeDescSet.descriptorType = descriptorType;
        writeDescSet.dstBinding = dstBinding;
        writeDescSet.pImageInfo = pBuffepImageInforInfo;
        writeDescSet.descriptorCount = descriptorCount;
        return writeDescSet;
    }

    VkCommandBufferBeginInfo cmdBufferBegin(VkCommandBufferUsageFlags flags) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;
        return beginInfo;
    }

    void printVec3(const glm::vec3& vector) {
        std::cout << vector.x << " " << vector.y << " " << vector.z << std::endl;
    }

    void printMat4(const glm::mat4& mat) {
        std::cout << "MAT4----------------" << std::endl;
        for (int i = 0; i < 4; ++i) {
            std::cout << std::endl;
            for (int j = 0; j < 4; ++j) {
                std::cout << mat[i][j] << " ";
            }
        }
        std::cout << "MAT4----------------" << std::endl;
    }
}