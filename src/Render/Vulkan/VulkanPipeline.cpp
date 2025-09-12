#include "Render/Vulkan/VulkanPipeline.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"

namespace moe {
    void VulkanPipelineBuilder::clear() {
        inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};

        rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};

        colorBlendAttachment = {};

        multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

        pipelineLayout = {};

        depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

        rendering = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

        shaderStages.clear();
    }

    VkPipeline VulkanPipelineBuilder::build(VkDevice device) {
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.pNext = nullptr;

        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.pNext = nullptr;

        colorBlendState.logicOpEnable = VK_FALSE;
        colorBlendState.logicOp = VK_LOGIC_OP_COPY;
        colorBlendState.attachmentCount = 1;
        colorBlendState.pAttachments = &colorBlendAttachment;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.pNext = nullptr;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &rendering;

        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlendState;
        pipelineInfo.pDepthStencilState = &depthStencil;

        pipelineInfo.layout = pipelineLayout;

        auto dynamicStates = Array<VkDynamicState, 2>{
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.pDynamicStates = dynamicStates.data();
        dynamicStateInfo.dynamicStateCount = dynamicStates.size();

        pipelineInfo.pDynamicState = &dynamicStateInfo;

        VkPipeline pipeline;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            Logger::warn("Pipeline creation failed");
            return VK_NULL_HANDLE;
        }
        return pipeline;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::addShader(VkShaderModule vert, VkShaderModule frag) {
        shaderStages.clear();

        shaderStages.push_back(
                VkInit::pipelineShaderStageCreateInfo(
                        VK_SHADER_STAGE_VERTEX_BIT, vert));
        shaderStages.push_back(
                VkInit::pipelineShaderStageCreateInfo(
                        VK_SHADER_STAGE_FRAGMENT_BIT, frag));

        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setInputTopology(VkPrimitiveTopology topology) {
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setPolygonMode(VkPolygonMode mode) {
        rasterizer.polygonMode = mode;
        rasterizer.lineWidth = 1.f;

        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setCullMode(VkCullModeFlags mode, VkFrontFace frontFace) {
        rasterizer.cullMode = mode;
        rasterizer.frontFace = frontFace;

        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::disableMultisampling() {
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::disableBlending() {
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        return *this;
    }
    VulkanPipelineBuilder& VulkanPipelineBuilder::enableBlending(
            VkBlendFactor srcColorBlendFactor,
            VkBlendFactor dstColorBlendFactor,
            VkBlendFactor srcAlphaBlendFactor,
            VkBlendFactor dstAlphaBlendFactor,
            VkBlendOp blendOp,
            VkColorComponentFlags colorWriteMask) {
        colorBlendAttachment.blendEnable = VK_TRUE;

        colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
        colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
        colorBlendAttachment.colorBlendOp = blendOp;

        colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
        colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
        colorBlendAttachment.alphaBlendOp = blendOp;

        colorBlendAttachment.colorWriteMask = colorWriteMask;

        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setColorAttachmentFormat(VkFormat format) {
        colorAttachmentFormat = format;
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &colorAttachmentFormat;

        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setDepthFormat(VkFormat format) {
        rendering.depthAttachmentFormat = format;

        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::disableDepthTesting() {
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};
        depthStencil.minDepthBounds = 0.f;
        depthStencil.maxDepthBounds = 1.f;

        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::enableDepthTesting(bool depthWriteEnabled, VkCompareOp compareOp) {
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = depthWriteEnabled;
        depthStencil.depthCompareOp = compareOp;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};
        depthStencil.minDepthBounds = 0.f;
        depthStencil.maxDepthBounds = 1.f;

        return *this;
    }
}// namespace moe