#include "Render/Vulkan/VulkanPostFXGraph.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"
#include "Render/Vulkan/VulkanUtils.hpp"


MOE_BEGIN_NAMESPACE

void VulkanPostFXGraph::destroy() {
    MOE_ASSERT(initialized, "VulkanPostFXGraph not initialized");

    // images managed by image cache, no need to destroy here
    nameToImageIndex.clear();
    compiledStages.clear();
    stages.clear();
    globalInputInfos.clear();
    globalOutputInfo = {};
    initialized = false;
    engine = nullptr;
}

void VulkanPostFXGraph::copyToInput(
        VkCommandBuffer cmdBuffer, String inputName,
        VulkanAllocatedImage& src, VkImageLayout srcImageLayout) {
    MOE_ASSERT(initialized, "VulkanPostFXGraph not initialized");

    VkUtils::transitionImage(
            cmdBuffer, src.image,
            srcImageLayout,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    auto it = nameToImageIndex.find(inputName);
    MOE_ASSERT(it != nameToImageIndex.end(), "Input name not found in post FX graph");

    auto& dstImage = compiledStages[it->second].outputImage;
    auto srcExtent = VkExtent2D{src.imageExtent.width, src.imageExtent.height};
    auto dstExtent = VkExtent2D{dstImage.imageExtent.width, dstImage.imageExtent.height};

    VkUtils::transitionImage(
            cmdBuffer, dstImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkUtils::copyImage(cmdBuffer, src.image, dstImage.image, srcExtent, dstExtent);

    VkUtils::transitionImage(
            cmdBuffer, dstImage.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkUtils::transitionImage(
            cmdBuffer, src.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            srcImageLayout);
}

void VulkanPostFXGraph::copyFromOutput(VkCommandBuffer cmdBuffer, VulkanAllocatedImage& dst, VkImageLayout dstImageLayout) {
    MOE_ASSERT(initialized, "VulkanPostFXGraph not initialized");

    VkUtils::transitionImage(
            cmdBuffer, dst.image,
            dstImageLayout,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    auto it = nameToImageIndex.find(globalOutputInfo.name);
    MOE_ASSERT(it != nameToImageIndex.end(), "Output name not found in post FX graph");

    auto& srcImage = compiledStages[it->second].outputImage;
    auto srcExtent = VkExtent2D{srcImage.imageExtent.width, srcImage.imageExtent.height};
    auto dstExtent = VkExtent2D{dst.imageExtent.width, dst.imageExtent.height};

    VkUtils::transitionImage(
            cmdBuffer, srcImage.image,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkUtils::copyImage(cmdBuffer, srcImage.image, dst.image, srcExtent, dstExtent);

    VkUtils::transitionImage(
            cmdBuffer, dst.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            dstImageLayout);

    VkUtils::transitionImage(
            cmdBuffer, srcImage.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanPostFXGraph::addGlobalInputName(StringView name, VkFormat format, VkExtent3D extent) {
    globalInputInfos.push_back(GlobalInputOutputInfo{String(name), format, extent});
    stages.push_back(VulkanPostFXStage{
            .name = String(name),
            .outputFormat = format,
            .outputExtent = extent,
            .isGlobalInput = true,
            .recordFunc = nullptr,
    });
}

void VulkanPostFXGraph::buildGraph() {
    for (auto& stage: stages) {
        uint32_t index = static_cast<uint32_t>(compiledStages.size());

        VulkanCompiledPostFXStage compiledStage;
        compiledStage.index = index;
        compiledStage.recordFunc = stage.recordFunc;
        compiledStage.outputFormat = stage.outputFormat;
        compiledStage.outputExtent = stage.outputExtent;

        nameToImageIndex[stage.name] = index;
        auto imageId =
                engine->m_caches.imageCache.addImage(engine->allocateImage(
                        stage.outputExtent, stage.outputFormat,
                        VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        false));

        auto image =
                engine->m_caches.imageCache.getImage(imageId).value();

        compiledStages.emplace(
                index,
                VulkanCompiledPostFXStage{
                        .index = index,
                        .outputImage = std::move(image),
                        .outputImageId = imageId,
                        .outputFormat = stage.outputFormat,
                        .outputExtent = stage.outputExtent,
                        .isGlobalInput = stage.isGlobalInput,
                        .recordFunc = stage.recordFunc,
                });
    }

    for (auto& [index, compiledStage]: compiledStages) {
        auto& stage = stages[index];
        for (auto& inputName: stage.inputNames) {
            auto it = nameToImageIndex.find(inputName);
            if (it == nameToImageIndex.end()) {
                Logger::critical("Input name {} not found in post FX graph", inputName);
            }
            MOE_ASSERT(it != nameToImageIndex.end(), "Input name not found in post FX graph");

            compiledStage.inputIndices.push_back(it->second);
            compiledStages[it->second].outputIndices.push_back(compiledStage.index);
        }
    }

    {
        auto it = nameToImageIndex.find(globalOutputInfo.name);
        if (it == nameToImageIndex.end()) {
            Logger::critical("Output name {} not found in post FX graph, this should be added as a stage output", globalOutputInfo.name);
        }
        MOE_ASSERT(it != nameToImageIndex.end(), "Output name not found in post FX graph, this should be added as a stage output");
    }
}

void VulkanPostFXGraph::resolveDependencies() {
    const size_t stageCount = compiledStages.size();
    Vector<size_t> inDegree(stageCount, 0);

    for (auto& [index, compiledStage]: compiledStages) {
        for (auto& outputIndex: compiledStage.outputIndices) {
            inDegree[outputIndex]++;
        }
    }

    Queue<uint32_t> zeroInDegreeQueue;
    for (size_t i = 0; i < stageCount; ++i) {
        if (inDegree[i] == 0) {
            zeroInDegreeQueue.push(i);
        }
    }

    while (!zeroInDegreeQueue.empty()) {
        auto stage = zeroInDegreeQueue.front();
        zeroInDegreeQueue.pop();
        compiledStagesOrder.push_back(stage);

        for (auto& outputIndex: compiledStages[stage].outputIndices) {
            inDegree[outputIndex]--;
            if (inDegree[outputIndex] == 0) {
                zeroInDegreeQueue.push(outputIndex);
            }
        }
    }

    MOE_ASSERT(
            compiledStagesOrder.size() == stageCount,
            "Cyclic dependency detected in post FX graph");
}


void VulkanPostFXGraph::logCompilationResult() {
    if (!enableCompilationLog) {
        return;
    }

    std::stringstream ss;
    ss << "[Post FX Graph Compilation Result]\n";
    ss << "Global Inputs:\n";
    for (auto& globalInput: globalInputInfos) {
        ss << " " << globalInput.name << "\n";
    }

    ss << "Stages (Sorted): " << compiledStages.size() << "\n";
    for (auto idx: compiledStagesOrder) {
        auto& stage = stages[idx];
        ss << " Stage: " << stage.name << "\n";
        if (!stage.isGlobalInput) {
            ss << "  Inputs: [";
            for (auto& inputIndex: compiledStages[idx].inputIndices) {
                for (auto& [name, index]: nameToImageIndex) {
                    if (index == inputIndex) {
                        ss << name << ", ";
                        break;
                    }
                }
            }
            ss << "]\n";
            ss << "  Resolved Outputs: [";
            for (auto& outputIndex: compiledStages[idx].outputIndices) {
                for (auto& [name, index]: nameToImageIndex) {
                    if (index == outputIndex) {
                        ss << name << ", ";
                        break;
                    }
                }
            }
            ss << "]";
        } else {
            ss << "  (Global Input Stage)";
        }
        ss << "\n";
    }

    ss << "Output Stage:";
    ss << " " << globalOutputInfo.name;
    ss << "\n";

    compilationLog = ss.str();
}

void VulkanPostFXGraph::compile() {
    compiledStages.clear();
    nameToImageIndex.clear();

    buildGraph();
    resolveDependencies();

    logCompilationResult();
}


void VulkanPostFXGraph::exec(VkCommandBuffer cmdBuffer) {
    MOE_ASSERT(initialized, "VulkanPostFXGraph not initialized");

    for (auto& index: compiledStagesOrder) {
        auto& compiledStage = compiledStages[index];
        if (compiledStage.isGlobalInput) {
            continue;
        }

        Vector<ImageId> inputImages;

        for (auto& inputIndex: compiledStage.inputIndices) {
            inputImages.push_back(compiledStages[inputIndex].outputImageId);
        }

        auto& outputImage = compiledStage.outputImage;
        VkUtils::transitionImage(
                cmdBuffer, outputImage.image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        auto clearValue = VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
        auto colorAttachment =
                VkInit::renderingAttachmentInfo(
                        outputImage.imageView,
                        &clearValue, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        auto renderInfo = VkInit::renderingInfo(
                {compiledStage.outputExtent.width, compiledStage.outputExtent.height},
                &colorAttachment, nullptr);
        vkCmdBeginRendering(cmdBuffer, &renderInfo);
        compiledStage.recordFunc(cmdBuffer, inputImages);
        vkCmdEndRendering(cmdBuffer);

        VkUtils::transitionImage(
                cmdBuffer, outputImage.image,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

MOE_END_NAMESPACE