#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

namespace moe {
    class VulkanPipelineBuilder {
    public:
        Vector<VkPipelineShaderStageCreateInfo> shaderStages;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly;
        VkPipelineRasterizationStateCreateInfo rasterizer;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo multisampling;
        VkPipelineLayout pipelineLayout;
        VkPipelineDepthStencilStateCreateInfo depthStencil;
        VkPipelineRenderingCreateInfo rendering;
        Array<VkFormat, 16> colorAttachmentFormats;
        size_t colorAttachmentCount{0};

        explicit VulkanPipelineBuilder(VkPipelineLayout layout) {
            clear();
            pipelineLayout = layout;
        }

        void clear();

        VkPipeline build(VkDevice device);

        VulkanPipelineBuilder& addShader(VkShaderModule vert, VkShaderModule frag);

        VulkanPipelineBuilder& setInputTopology(VkPrimitiveTopology topology);

        VulkanPipelineBuilder& setPolygonMode(VkPolygonMode mode);

        VulkanPipelineBuilder& setCullMode(VkCullModeFlags mode, VkFrontFace frontFace);

        // todo: multisampling
        VulkanPipelineBuilder& disableMultisampling();

        VulkanPipelineBuilder& enableMultisampling(VkSampleCountFlagBits samples);

        VulkanPipelineBuilder& disableBlending();

        VulkanPipelineBuilder& enableBlending(
                VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                VkBlendOp blendOp = VK_BLEND_OP_ADD,
                VkColorComponentFlags colorWriteMask =
                        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

        VulkanPipelineBuilder& setColorAttachmentFormat(VkFormat format);

        VulkanPipelineBuilder& addColorAttachmentFormat(VkFormat format);

        VulkanPipelineBuilder& setDepthFormat(VkFormat format);

        VulkanPipelineBuilder& disableDepthTesting();

        VulkanPipelineBuilder& enableDepthTesting(bool depthWriteEnabled, VkCompareOp compareOp);
    };

    class VulkanComputePipelineBuilder {
    public:
        explicit VulkanComputePipelineBuilder(VkPipelineLayout layout) {
            pipelineLayout = layout;
        }

        VulkanComputePipelineBuilder& setShader(VkShaderModule computeShader) {
            shaderStage.module = computeShader;
            shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            return *this;
        }

        VkPipeline build(VkDevice device) {
            VkComputePipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.layout = pipelineLayout;
            pipelineInfo.stage = shaderStage;

            VkPipeline pipeline;
            VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
            if (result != VK_SUCCESS) {
                Logger::error("Failed to create compute pipeline");
                return VK_NULL_HANDLE;
            }
            return pipeline;
        }

    private:
        VkPipelineLayout pipelineLayout;
        VkPipelineShaderStageCreateInfo shaderStage;
    };
}// namespace moe