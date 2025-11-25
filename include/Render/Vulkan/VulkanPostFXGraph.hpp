#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

#include "Core/Meta/TypeTraits.hpp"

MOE_BEGIN_NAMESPACE

struct VulkanEngine;

struct VulkanPostFXStage {
    Vector<String> inputNames;
    String name;
    VkFormat outputFormat;
    VkExtent3D outputExtent;

    bool isGlobalInput{false};

    Function<void(VkCommandBuffer, Vector<ImageId>&)> recordFunc;
};

struct VulkanCompiledPostFXStage {
    uint32_t index;

    Vector<uint32_t> inputIndices;
    Vector<uint32_t> outputIndices;

    VulkanAllocatedImage outputImage;
    ImageId outputImageId;

    VkFormat outputFormat;
    VkExtent3D outputExtent;

    bool isGlobalInput{false};

    Function<void(VkCommandBuffer, Vector<ImageId>&)> recordFunc;
};

struct VulkanPostFXGraph {
    void init(VulkanEngine& eng) {
        MOE_ASSERT(!initialized, "VulkanPostFXGraph already initialized");
        engine = &eng;
        initialized = true;
    }

    void destroy();

    void copyToInput(VkCommandBuffer cmdBuffer, String inputName, VulkanAllocatedImage& src, VkImageLayout srcImageLayout);
    void copyFromOutput(VkCommandBuffer cmdBuffer, VulkanAllocatedImage& dst, VkImageLayout dstImageLayout);

    void addGlobalInputName(StringView name, VkFormat format, VkExtent3D extent);

    void setGlobalOutputName(StringView name) {
        globalOutputInfo = {.name = String(name)};
    }

    void addStage(VulkanPostFXStage&& stage) {
        stages.push_back(std::move(stage));
    }

    template<
            typename Fn,
            typename InputNames,
            typename = Meta::EnableIf<Meta::IsInvocable<Fn, void(VkCommandBuffer, Vector<ImageId>&)>::value>,
            typename = Meta::EnableIf<std::is_convertible_v<InputNames, StringView>>>
    void addStage(StringView name, std::initializer_list<InputNames> inputNames,
                  VkFormat outputFormat, VkExtent3D outputExtent,
                  Fn&& recordFunc) {
        VulkanPostFXStage stage;
        stage.name = String(name);
        stage.outputFormat = outputFormat;
        stage.outputExtent = outputExtent;
        stage.recordFunc = std::forward<Fn>(recordFunc);
        for (auto& inName: inputNames) {
            stage.inputNames.push_back(String(inName));
        }
        stages.push_back(std::move(stage));
    }

    void compile();

    void exec(VkCommandBuffer cmdBuffer);

    void setCompilationLogEnabled(bool enabled) {
        enableCompilationLog = enabled;
    }

    StringView getCompilationLog() const {
        return compilationLog;
    }

private:
    Vector<VulkanPostFXStage> stages;
    Vector<uint32_t> compiledStagesOrder;

    struct GlobalInputOutputInfo {
        String name;
        VkFormat format;
        VkExtent3D extent;
    };

    Vector<GlobalInputOutputInfo> globalInputInfos;
    GlobalInputOutputInfo globalOutputInfo;

    UnorderedMap<uint32_t, VulkanCompiledPostFXStage> compiledStages;
    UnorderedMap<String, uint32_t> nameToImageIndex;

    VulkanEngine* engine{nullptr};
    bool initialized{false};

    String compilationLog;
    bool enableCompilationLog{false};

    void buildGraph();

    void resolveDependencies();

    void logCompilationResult();
};

MOE_END_NAMESPACE