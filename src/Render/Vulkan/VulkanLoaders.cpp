#include "Render/Vulkan/VulkanLoaders.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanScene.hpp"

#include <tiny_gltf.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


namespace moe {
    namespace VkLoaders {
        UniqueRawImage loadImage(StringView filename, int* width, int* height, int* channels, int desiredChannels) {
            auto* buf = stbi_load(filename.data(), width, height, channels, desiredChannels);
            //MOE_ASSERT(*channels == 4, "Only 4-channel images are supported");

            // todo: implement more robust image loading

            if (!buf) {
                Logger::error("Failed to load image: {}", filename);
                return nullptr;
            }

            return UniqueRawImage(buf);
        }

        namespace GLTF {
            // ! note: this part uses tinygltf instead of fastgltf
            // ! for that fastgltf is too complicated for me
            // ! based on: https://github.com/eliasdaler/edbr/blob/master/edbr/src/Util/GltfLoader.cpp
            // ! very nice glTF loader implementation!

            // todo: implement binary file (.glb) support

            static const std::string GLTF_POSITIONS_ACCESSOR{"POSITION"};
            static const std::string GLTF_NORMALS_ACCESSOR{"NORMAL"};
            static const std::string GLTF_TANGENTS_ACCESSOR{"TANGENT"};
            static const std::string GLTF_UVS_ACCESSOR{"TEXCOORD_0"};
            static const std::string GLTF_JOINTS_ACCESSOR{"JOINTS_0"};
            static const std::string GLTF_WEIGHTS_ACCESSOR{"WEIGHTS_0"};

            static const std::string GLTF_SAMPLER_PATH_TRANSLATION{"translation"};
            static const std::string GLTF_SAMPLER_PATH_ROTATION{"rotation"};
            static const std::string GLTF_SAMPLER_PATH_SCALE{"scale"};

            // illumination extension
            static const std::string GLTF_LIGHTS_PUNCTUAL_KEY{"KHR_lights_punctual"};
            static const std::string GLTF_LIGHTS_PUNCTUAL_POINT_NAME{"point"};
            static const std::string GLTF_LIGHTS_PUNCTUAL_DIRECTIONAL_NAME{"directional"};
            static const std::string GLTF_LIGHTS_PUNCTUAL_SPOT_NAME{"spot"};

            static const std::string GLTF_EMISSIVE_STRENGTH_KHR_KEY{"KHR_materials_emissive_strength"};
            static const std::string GLTF_EMISSIVE_STRENGTH_PARAM_NAME{"emissiveStrength"};

            glm::vec3 cvtTinyGltfVec3(const std::vector<double>& vec) {
                assert(vec.size() == 3);
                return {vec[0], vec[1], vec[2]};
            }

            glm::vec4 cvtTinyGltfVec4(const std::vector<double>& vec) {
                if (vec.size() == 4) {
                    return {vec[0], vec[1], vec[2], vec[3]};
                }

                assert(vec.size() == 3);
                return {vec[0], vec[1], vec[2], 1.f};
            }

            glm::quat cvtTinyGltfQuat(const std::vector<double>& vec) {
                assert(vec.size() == 4);
                return {
                        static_cast<float>(vec[3]),
                        static_cast<float>(vec[0]),
                        static_cast<float>(vec[1]),
                        static_cast<float>(vec[2]),
                };
            }

            void loadGltfFile(tinygltf::Model& model, std::filesystem::path path) {
                tinygltf::TinyGLTF loader;
                std::string err;
                std::string warn;

                bool success = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
                if (!warn.empty()) {
                    Logger::warn("GLTF loader warning: {}", warn);
                }

                if (!success) {
                    Logger::error("Failed to load glTF: {}", err);
                    MOE_ASSERT(false, "Failed to load glTF");
                }
            }

            int findAttributeAccessor(const tinygltf::Primitive& primitive, StringView attributeName) {
                for (const auto& [accessorName, accessorID]: primitive.attributes) {
                    if (accessorName == attributeName) {
                        return accessorID;
                    }
                }
                return -1;
            }

            bool hasAccessor(const tinygltf::Primitive& primitive, StringView attributeName) {
                const auto accessorIndex = findAttributeAccessor(primitive, attributeName);
                return accessorIndex != -1;
            }

            template<typename T>
            Span<const T> getPackedBufferSpan(
                    const tinygltf::Model& model,
                    const tinygltf::Accessor& accessor) {
                const auto& bv = model.bufferViews[accessor.bufferView];
                const int bs = accessor.ByteStride(bv);
                if (bs != sizeof(T)) {
                    Logger::error("Accessor buffer is not tightly packed, expected {}, got {}", sizeof(T), bs);
                    MOE_ASSERT(false, "Accessor buffer is not tightly packed");
                }

                const auto& buf = model.buffers[bv.buffer];
                const auto* data =
                        reinterpret_cast<const T*>(&buf.data.at(0) + bv.byteOffset + accessor.byteOffset);
                return Span<const T>{data, accessor.count};
            }

            template<typename T>
            Span<const T> getPackedBufferSpan(
                    const tinygltf::Model& model,
                    const tinygltf::Primitive& primitive,
                    const StringView attributeName) {
                const auto accessorIndex = findAttributeAccessor(primitive, attributeName);
                assert(accessorIndex != -1 && "Accessor not found");
                const auto& accessor = model.accessors[accessorIndex];
                return getPackedBufferSpan<T>(model, accessor);
            }

            glm::vec4 getColor(const tinygltf::Material& gltfMaterial) {
                auto& color = gltfMaterial.pbrMetallicRoughness.baseColorFactor;
                if (color.size() == 4) {
                    return glm::vec4(color[0], color[1], color[2], color[3]);
                } else {
                    Logger::error("glTF material has no color");
                    return glm::vec4(1.0f);
                }
            }
            bool hasDiffuseTexture(const tinygltf::Material& material) {
                const auto textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                return textureIndex != -1;
            }

            bool hasNormalMapTexture(const tinygltf::Material& material) {
                const auto textureIndex = material.normalTexture.index;
                return textureIndex != -1;
            }

            bool hasMetallicRoughnessTexture(const tinygltf::Material& material) {
                const auto textureIndex = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
                return textureIndex != -1;
            }

            bool hasEmissiveTexture(const tinygltf::Material& material) {
                const auto textureIndex = material.emissiveTexture.index;
                return textureIndex != -1;
            }

            std::filesystem::path getDiffuseTexturePath(
                    const tinygltf::Model& model,
                    const tinygltf::Material& material,
                    const std::filesystem::path& fileDir) {
                const auto textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                const auto& textureId = model.textures[textureIndex];
                const auto& image = model.images[textureId.source];
                return fileDir / image.uri;
            }
            std::filesystem::path getNormalMapTexturePath(
                    const tinygltf::Model& model,
                    const tinygltf::Material& material,
                    const std::filesystem::path& fileDir) {
                const auto textureIndex = material.normalTexture.index;
                const auto& textureId = model.textures[textureIndex];
                const auto& image = model.images[textureId.source];
                return fileDir / image.uri;
            }

            std::filesystem::path getMetallicRoughnessTexturePath(
                    const tinygltf::Model& model,
                    const tinygltf::Material& material,
                    const std::filesystem::path& fileDir) {
                const auto textureIndex = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
                const auto& textureId = model.textures[textureIndex];
                const auto& image = model.images[textureId.source];
                return fileDir / image.uri;
            }

            std::filesystem::path getEmissiveTexturePath(
                    const tinygltf::Model& model,
                    const tinygltf::Material& material,
                    const std::filesystem::path& fileDir) {
                const auto textureIndex = material.emissiveTexture.index;
                const auto& textureId = model.textures[textureIndex];
                const auto& image = model.images[textureId.source];
                return fileDir / image.uri;
            }

            float getEmissiveStrength(const tinygltf::Material& material) {
                if (material.extensions.find(GLTF_EMISSIVE_STRENGTH_KHR_KEY) != material.extensions.end()) {
                    return (float) material.extensions.at(GLTF_EMISSIVE_STRENGTH_KHR_KEY)
                            .Get(GLTF_EMISSIVE_STRENGTH_PARAM_NAME)
                            .GetNumberAsDouble();
                }
                return 1.0f;
            }

            VulkanCPUMaterial loadMaterial(
                    VulkanImageCache& imageCache,
                    const tinygltf::Model& gltfModel,
                    const tinygltf::Material& gltfMaterial,
                    const std::filesystem::path& parentPath) {
                VulkanCPUMaterial material{};
                material.baseColor = getColor(gltfMaterial);
                material.metallic = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
                material.roughness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);

                if (hasDiffuseTexture(gltfMaterial)) {
                    const auto diffusePath = getDiffuseTexturePath(gltfModel, gltfMaterial, parentPath);
                    material.diffuseTexture =
                            imageCache.loadImageFromFile(diffusePath.string(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, true);
                } else {
                    material.diffuseTexture = imageCache.getDefaultImage(VulkanImageCache::DefaultResourceType::White);
                }

                if (hasNormalMapTexture(gltfMaterial)) {
                    const auto normalMapPath = getNormalMapTexturePath(gltfModel, gltfMaterial, parentPath);
                    material.normalTexture =
                            imageCache.loadImageFromFile(normalMapPath.string(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, true);
                } else {
                    material.normalTexture = imageCache.getDefaultImage(VulkanImageCache::DefaultResourceType::FlatNormal);
                }

                if (hasMetallicRoughnessTexture(gltfMaterial)) {
                    const auto metalRoughnessPath =
                            getMetallicRoughnessTexturePath(gltfModel, gltfMaterial, parentPath);
                    material.metallicRoughnessTexture = imageCache.loadImageFromFile(metalRoughnessPath.string(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, true);
                } else {
                    material.metallicRoughnessTexture = imageCache.getDefaultImage(VulkanImageCache::DefaultResourceType::White);
                }

                material.emissive = getEmissiveStrength(gltfMaterial);
                material.emissiveColor = cvtTinyGltfVec4(gltfMaterial.emissiveFactor);
                if (hasEmissiveTexture(gltfMaterial)) {
                    const auto emissivePath =
                            getEmissiveTexturePath(gltfModel, gltfMaterial, parentPath);
                    material.emissiveTexture = imageCache.loadImageFromFile(emissivePath.string(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, true);
                } else {
                    // if the object is not emissive, use white texture
                    material.emissiveTexture = imageCache.getDefaultImage(VulkanImageCache::DefaultResourceType::White);
                }

                return material;
            }

            VulkanCPUMesh loadPrimitive(
                    const tinygltf::Model& model,
                    const tinygltf::Primitive& primitive) {
                VulkanCPUMesh mesh{};

                if (primitive.indices != -1) {// load indices
                    const auto& indexAccessor = model.accessors[primitive.indices];
                    switch (indexAccessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                            MOE_ASSERT(indexAccessor.type == TINYGLTF_TYPE_SCALAR, "Invalid index accessor type");
                            {
                                const auto indices = getPackedBufferSpan<std::uint32_t>(model, indexAccessor);
                                mesh.indices.assign(indices.begin(), indices.end());
                            }
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            MOE_ASSERT(indexAccessor.type == TINYGLTF_TYPE_SCALAR, "Invalid index accessor type");
                            {
                                const auto indices = getPackedBufferSpan<std::uint16_t>(model, indexAccessor);
                                mesh.indices.assign(indices.begin(), indices.end());
                            }
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            MOE_ASSERT(indexAccessor.type == TINYGLTF_TYPE_SCALAR, "Invalid index accessor type");
                            {
                                const auto indices = getPackedBufferSpan<std::uint8_t>(model, indexAccessor);
                                mesh.indices.reserve(indices.size());
                                for (auto i: indices) {
                                    mesh.indices.push_back(i);
                                }
                            }
                            break;
                        default:
                            MOE_ASSERT(false, "Unsupported index component type");
                    }
                }

                // load positions
                const auto positions =
                        getPackedBufferSpan<glm::vec3>(model, primitive, GLTF_POSITIONS_ACCESSOR);
                mesh.vertices.resize(positions.size());
                for (std::size_t i = 0; i < positions.size(); ++i) {
                    mesh.vertices[i].pos = positions[i];
                }

                {
                    // get min and max pos
                    const auto posAccessorIndex = findAttributeAccessor(primitive, GLTF_POSITIONS_ACCESSOR);
                    MOE_ASSERT(posAccessorIndex != -1, "Accessor not found");
                    const auto& posAccessor = model.accessors[posAccessorIndex];
                    mesh.min = cvtTinyGltfVec3(posAccessor.minValues);
                    mesh.max = cvtTinyGltfVec3(posAccessor.maxValues);
                }

                const auto numVertices = positions.size();

                // load normals
                if (hasAccessor(primitive, GLTF_NORMALS_ACCESSOR)) {
                    const auto normals =
                            getPackedBufferSpan<glm::vec3>(model, primitive, GLTF_NORMALS_ACCESSOR);
                    MOE_ASSERT(normals.size() == numVertices, "Invalid number of normals");
                    for (std::size_t i = 0; i < normals.size(); ++i) {
                        mesh.vertices[i].normal = normals[i];
                        // ! TEMP
                    }
                }

                // load tangents
                if (hasAccessor(primitive, GLTF_TANGENTS_ACCESSOR)) {
                    const auto tangents =
                            getPackedBufferSpan<glm::vec4>(model, primitive, GLTF_TANGENTS_ACCESSOR);
                    MOE_ASSERT(tangents.size() == numVertices, "Invalid number of tangents");
                    for (std::size_t i = 0; i < tangents.size(); ++i) {
                        mesh.vertices[i].tangent = tangents[i];
                    }
                }

                // load uvs
                if (hasAccessor(primitive, GLTF_UVS_ACCESSOR)) {
                    const auto uvs = getPackedBufferSpan<glm::vec2>(model, primitive, GLTF_UVS_ACCESSOR);
                    MOE_ASSERT(uvs.size() == numVertices, "Invalid number of uvs");
                    for (std::size_t i = 0; i < uvs.size(); ++i) {
                        mesh.vertices[i].uv_x = uvs[i].x;// TEMP
                        mesh.vertices[i].uv_y = uvs[i].y;// TEMP
                    }
                }

                if (hasAccessor(primitive, GLTF_JOINTS_ACCESSOR)) {
                    MOE_ASSERT(hasAccessor(primitive, GLTF_WEIGHTS_ACCESSOR), "Joints accessor found without weights accessor");

                    auto jointsAccessorIndex = findAttributeAccessor(primitive, GLTF_JOINTS_ACCESSOR);
                    auto weightsAccessorIndex = findAttributeAccessor(primitive, GLTF_WEIGHTS_ACCESSOR);

                    const auto& jointsAccessor = model.accessors[jointsAccessorIndex];
                    const auto& weightsAccessor = model.accessors[weightsAccessorIndex];

                    mesh.hasSkeleton = true;
                    mesh.skinningData.resize(numVertices);

                    {
                        switch (jointsAccessor.componentType) {
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                                const auto joints = getPackedBufferSpan<std::uint8_t[4]>(model, jointsAccessor);
                                MOE_ASSERT(joints.size() == numVertices, "Invalid number of joints");

                                for (std::size_t i = 0; i < joints.size(); ++i) {
                                    mesh.skinningData[i].jointIds =
                                            glm::uvec4{joints[i][0], joints[i][1], joints[i][2], joints[i][3]};
                                }
                                break;
                            }
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                                const auto joints =
                                        getPackedBufferSpan<std::uint16_t[4]>(model, jointsAccessor);
                                MOE_ASSERT(joints.size() == numVertices, "Invalid number of joints");


                                for (std::size_t i = 0; i < joints.size(); ++i) {
                                    mesh.skinningData[i].jointIds =
                                            glm::uvec4{joints[i][0], joints[i][1], joints[i][2], joints[i][3]};
                                }
                                break;
                            }
                            default:
                                MOE_ASSERT(false, "Unsupported joints accessor component type");
                        }
                    }

                    {
                        switch (weightsAccessor.componentType) {
                            case TINYGLTF_COMPONENT_TYPE_FLOAT: {
                                const auto weights =
                                        getPackedBufferSpan<float[4]>(model, weightsAccessor);
                                MOE_ASSERT(weights.size() == numVertices, "Invalid number of weights");

                                for (std::size_t i = 0; i < weights.size(); ++i) {
                                    mesh.skinningData[i].weights =
                                            glm::vec4{weights[i][0], weights[i][1], weights[i][2], weights[i][3]};
                                }
                                break;
                            }
                            case TINYGLTF_COMPONENT_TYPE_DOUBLE: {
                                const auto weights =
                                        getPackedBufferSpan<double[4]>(model, weightsAccessor);
                                MOE_ASSERT(weights.size() == numVertices, "Invalid number of weights");

                                for (std::size_t i = 0; i < weights.size(); ++i) {
                                    mesh.skinningData[i].weights =
                                            glm::vec4{weights[i][0], weights[i][1], weights[i][2], weights[i][3]};
                                }
                                break;
                            }
                            default:
                                MOE_ASSERT(false, "Unsupported weights accessor component type");
                        }
                    }
                }

                return mesh;
            }

            glm::mat4 loadTransform(const tinygltf::Node& node) {
                glm::mat4 transform(1.0f);

                if (!node.matrix.empty()) {
                    for (int i = 0; i < 16; ++i) {
                        transform[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
                    }
                    return transform;
                }

                glm::mat4 T(1.0f), R(1.0f), S(1.0f);
                if (!node.scale.empty()) {
                    glm::vec3 scale = cvtTinyGltfVec3(node.scale);
                    S = glm::scale(S, scale);
                }

                if (!node.rotation.empty()) {
                    glm::quat rotation = cvtTinyGltfQuat(node.rotation);
                    R *= glm::toMat4(rotation);
                }

                if (!node.translation.empty()) {
                    glm::vec3 translation = cvtTinyGltfVec3(node.translation);
                    T = glm::translate(T, translation);
                }

                // ! glTF spec
                transform = T * R * S;

                return transform;
            }

            bool isMesh(const tinygltf::Node& node) {
                return node.mesh != -1;
            }


            VulkanSkeleton loadSkeleton(
                    UnorderedMap<int, JointId>& nodeIdxToJointId,
                    const tinygltf::Model& model,
                    const tinygltf::Skin& skin) {
                VulkanSkeleton skeleton{};

                const auto& inverseBindMatricesAccessor = model.accessors[skin.inverseBindMatrices];
                {
                    MOE_ASSERT(
                            inverseBindMatricesAccessor.type == TINYGLTF_TYPE_MAT4,
                            "Invalid inverse bind matrices accessor type");

                    switch (inverseBindMatricesAccessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_FLOAT: {
                            const auto inverseBindMatricesSpan = getPackedBufferSpan<glm::mat4>(model, inverseBindMatricesAccessor);
                            skeleton.inverseBindMatrices.reserve(inverseBindMatricesAccessor.count);
                            skeleton.inverseBindMatrices.assign(
                                    inverseBindMatricesSpan.begin(),
                                    inverseBindMatricesSpan.end());
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_DOUBLE: {
                            const auto inverseBindMatricesSpan = getPackedBufferSpan<glm::dmat4>(model, inverseBindMatricesAccessor);
                            skeleton.inverseBindMatrices.reserve(inverseBindMatricesAccessor.count);
                            skeleton.inverseBindMatrices.assign(
                                    inverseBindMatricesSpan.begin(),
                                    inverseBindMatricesSpan.end());
                            break;
                        }
                        default:
                            MOE_ASSERT(false, "Unsupported inverse bind matrices component type");
                            break;
                    }
                }

                size_t jointCount = skin.joints.size();
                skeleton.joints.reserve(jointCount);
                skeleton.jointNames.resize(jointCount);

                {
                    JointId jointId{0};
                    for (const auto& nodeIdx: skin.joints) {
                        nodeIdxToJointId.emplace(nodeIdx, jointId);

                        const auto& jointNode = model.nodes[nodeIdx];
                        skeleton.jointNames[jointId] = jointNode.name;

                        skeleton.joints.push_back(VulkanSkeleton::Joint{
                                .id = jointId,
                                .localTransform = loadTransform(jointNode),
                        });

                        ++jointId;
                    }
                }

                {
                    skeleton.hierarchy.resize(jointCount);
                    for (JointId jointId = 0; jointId < skeleton.joints.size(); ++jointId) {
                        const auto& jointNode = model.nodes[skin.joints[jointId]];
                        for (const auto& childIdx: jointNode.children) {
                            auto it = nodeIdxToJointId.find(childIdx);
                            if (it == nodeIdxToJointId.end()) {
                                continue;// not a joint
                            }
                            JointId childJointId = it->second;
                            skeleton.hierarchy[jointId].children.push_back(childJointId);
                        }
                    }
                }

                return skeleton;
            }

            UnorderedMap<String, AnimationId> loadAnimations(
                    const VulkanSkeleton& skeleton,
                    const UnorderedMap<int, JointId>& gltfNodeIdxToJointId,
                    const tinygltf::Model& gltfModel,
                    VulkanEngine& engine) {
                UnorderedMap<String, VulkanSkeletonAnimation> animations(gltfModel.animations.size());
                for (const auto& gltfAnimation: gltfModel.animations) {
                    Logger::debug("Loading animation: {}", gltfAnimation.name);

                    auto& animation = animations[gltfAnimation.name];
                    animation.name = gltfAnimation.name;

                    const auto numJoints = skeleton.joints.size();

                    animation.tracks.resize(numJoints);

                    for (const auto& channel: gltfAnimation.channels) {
                        const auto& sampler = gltfAnimation.samplers[channel.sampler];

                        MOE_ASSERT(gltfModel.accessors[sampler.input].count == gltfModel.accessors[sampler.output].count,
                                   "Input and output accessor count mismatch");

                        const auto& timesAccessor = gltfModel.accessors[sampler.input];
                        const auto times = getPackedBufferSpan<float>(gltfModel, timesAccessor);

                        auto duration = static_cast<float>(timesAccessor.maxValues[0] - timesAccessor.minValues[0]);
                        if (duration == 0) {
                            continue;// skip empty animations (e.g. keying sets)
                        }

                        if (channel.target_path == "weights") {
                            // ! fixme: implement morph target animation
                            // The "weights" target_path in glTF animation channels is used for morph target (blend shape) animations.
                            // It animates the weights of morph targets on a mesh, allowing for vertex-based deformations (e.g. facial expressions).
                            continue;
                        }

                        const auto nodeId = channel.target_node;
                        const auto jointId = gltfNodeIdxToJointId.at(nodeId);

                        auto& animationTrack = animation.tracks[jointId];

                        const auto& outputAccessor = gltfModel.accessors[sampler.output];
                        MOE_ASSERT(outputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Unsupported output component type");
                        if (channel.target_path == GLTF_SAMPLER_PATH_TRANSLATION) {
                            const auto translationKeys =
                                    getPackedBufferSpan<glm::vec3>(gltfModel, outputAccessor);

                            auto& tc = animationTrack.translations;

                            // in some cases this will have 2 identical keys, if no transform is applied.
                            // however we don't care,
                            // as this can simplify the interpolation implementation
                            tc.reserve(translationKeys.size());
                            tc.assign(translationKeys.begin(), translationKeys.end());

                            animationTrack.keyTimes.translations.reserve(times.size());
                            animationTrack.keyTimes.translations.assign(times.begin(), times.end());
                        } else if (channel.target_path == GLTF_SAMPLER_PATH_ROTATION) {
                            const auto rotationKeys = getPackedBufferSpan<glm::vec4>(gltfModel, outputAccessor);

                            auto& rc = animationTrack.rotations;
                            rc.reserve(rotationKeys.size());
                            for (const auto& qv: rotationKeys) {
                                const glm::quat q{qv.w, qv.x, qv.y, qv.z};
                                rc.push_back(q);
                            }

                            animationTrack.keyTimes.rotations.reserve(times.size());
                            animationTrack.keyTimes.rotations.assign(times.begin(), times.end());
                        } else if (channel.target_path == GLTF_SAMPLER_PATH_SCALE) {
                            const auto scaleKeys =
                                    getPackedBufferSpan<const glm::vec3>(gltfModel, outputAccessor);

                            auto& sc = animationTrack.scales;
                            sc.reserve(scaleKeys.size());
                            sc.assign(scaleKeys.begin(), scaleKeys.end());

                            animationTrack.keyTimes.scales.reserve(times.size());
                            animationTrack.keyTimes.scales.assign(times.begin(), times.end());
                        } else {
                            Logger::warn("Unsupported animation channel path: {}", channel.target_path);
                        }
                    }
                }

                UnorderedMap<String, AnimationId> animationNameToId{animations.size()};
                for (auto& [name, animation]: animations) {
                    AnimationId id = engine.m_caches.animationCache.load(std::move(animation)).first;
                    animationNameToId[name] = id;
                }

                return animationNameToId;
            }

            void loadNode(VulkanSceneNode& node, const tinygltf::Node& gltfNode, const tinygltf::Model& model) {
                node.localTransform = loadTransform(gltfNode);
                // MOE_ASSERT(gltfNode.mesh != -1, "Node is not a mesh");

                if (isMesh(gltfNode)) {
                    node.resourceInternalId = static_cast<GLTFInternalId>(gltfNode.mesh);
                    //Logger::debug("Loaded mesh: {}", gltfNode.name);
                } else {
                    node.resourceInternalId = NULL_SCENE_RESOURCE_INTERNAL_ID;
                    //Logger::debug("Loaded empty node: {}", gltfNode.name);
                }

                node.children.reserve(gltfNode.children.size());
                for (std::size_t childIdx = 0; childIdx < gltfNode.children.size(); ++childIdx) {
                    const auto& childNode = model.nodes[gltfNode.children[childIdx]];

                    auto childPtr = std::make_unique<VulkanSceneNode>();
                    auto& child = *static_cast<VulkanSceneNode*>(childPtr.get());
                    loadNode(child, childNode, model);

                    node.children.push_back(std::move(childPtr));
                }
            }

            Optional<VulkanScene> loadSceneFromFile(VulkanEngine& engine, StringView filename) {
                auto& meshCache = engine.m_caches.meshCache;
                auto& materialCache = engine.m_caches.materialCache;
                auto& imageCache = engine.m_caches.imageCache;

                const std::filesystem::path path = filename;
                const auto parentPath = path.parent_path();

                tinygltf::Model model;
                loadGltfFile(model, path);

                //Logger::debug("GLTF file contains {} scenes", model.scenes.size());
                auto& scene = model.scenes[model.defaultScene];
                UnorderedMap<GLTFInternalId, MaterialId> materialMap;

                {
                    for (GLTFInternalId i = 0; i < model.materials.size(); ++i) {
                        auto& mat = model.materials[i];
                        MaterialId id = materialCache.loadMaterial(loadMaterial(imageCache, model, mat, parentPath));
                        materialMap[i] = id;
                    }
                }

                VulkanScene vkScene{};
                for (const auto& mesh: model.meshes) {
                    VulkanSceneMesh vkMesh{};
                    vkMesh.primitives.resize(mesh.primitives.size());
                    vkMesh.primitiveMaterials.resize(mesh.primitives.size());

                    for (int i = 0; i < mesh.primitives.size(); ++i) {
                        const auto& primitive = mesh.primitives[i];
                        auto cpuMesh = loadPrimitive(model, primitive);
                        if (cpuMesh.indices.empty() || cpuMesh.vertices.empty()) {
                            continue;
                        }

                        auto meshId = meshCache.loadMesh(cpuMesh);
                        // cpuMesh.discard();
                        // discard the data to prevent extra memory usage
                        // ! fixme: after being discarded, other modules (e.g. physics) can't access the data
                        // ! find a proper place to perform this operation

                        MaterialId materialId = materialCache.getDefaultMaterial(VulkanMaterialCache::DefaultResourceType::White);

                        if (primitive.material != -1) {
                            materialId = materialMap.at(primitive.material);
                        }

                        vkMesh.primitives[i] = meshId;
                        vkMesh.primitiveMaterials[i] = materialId;
                    }

                    vkScene.meshes.push_back(vkMesh);
                }

                // ! fixme: only one skeleton is supported
                UnorderedMap<int, JointId> nodeIdxToJointId;
                vkScene.skeletons.reserve(model.skins.size());
                for (const auto& skin: model.skins) {
                    auto skeleton = loadSkeleton(nodeIdxToJointId, model, skin);
                    vkScene.skeletons.push_back(std::move(skeleton));
                }

                Logger::debug("Loaded {} skeletons", vkScene.skeletons.size());

                if (vkScene.skeletons.size() > 1) {
                    Logger::warn("Only one skeleton is supported, but {} skeletons found", vkScene.skeletons.size());
                }

                if (!vkScene.skeletons.empty()) {
                    vkScene.animations = loadAnimations(vkScene.skeletons[0], nodeIdxToJointId, model, engine);
                    Logger::debug("Loaded {} animations", vkScene.animations.size());
                }

                // todo: joint animation lighting

                vkScene.children.reserve(scene.nodes.size());
                for (std::size_t nodeIdx = 0; nodeIdx < scene.nodes.size(); ++nodeIdx) {
                    const auto& gltfNode = model.nodes[scene.nodes[nodeIdx]];

                    // ! the original imlementation from edbr.
                    // ! preserve for now
                    // HACK: load mesh with skin (for now only one assumed)
                    if (gltfNode.children.size() == 2) {
                        const auto& c1 = model.nodes[gltfNode.children[0]];
                        const auto& c2 = model.nodes[gltfNode.children[1]];
                        if ((c1.mesh != -1 && c1.skin != -1) || (c2.mesh != -1 && c2.skin != -1)) {
                            const auto& meshNode = (c1.mesh != -1) ? c1 : c2;

                            auto& nodePtr = vkScene.children[nodeIdx];
                            nodePtr = std::make_unique<VulkanSceneNode>();
                            auto& node = *static_cast<VulkanSceneNode*>(nodePtr.get());
                            loadNode(node, meshNode, model);

                            // sometimes the armature node can have scaling
                            // this is BAD, but we can't avoid it for models
                            const auto parentTransform = loadTransform(gltfNode);
                            node.localTransform = parentTransform * node.localTransform;

                            continue;
                        }
                    }

                    auto nodePtr = std::make_unique<VulkanSceneNode>();
                    auto& node = *static_cast<VulkanSceneNode*>(nodePtr.get());
                    loadNode(node, gltfNode, model);

                    vkScene.children.push_back(std::move(nodePtr));
                }

                return vkScene;
            }
        }// namespace GLTF
    }// namespace VkLoaders
}// namespace moe