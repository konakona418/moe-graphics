#include "Render/Vulkan/VulkanLoaders.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/util.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <stb_image.h>


namespace moe {
    namespace VkLoaders {
        Optional<VulkanMeshAssetGroup> loadGLTFMeshFromFile(VulkanEngine& engine, StringView filename) {
            std::filesystem::path path = filename;

            fastgltf::GltfDataBuffer buf;
            buf.FromPath(path);
            constexpr auto options = fastgltf::Options::LoadExternalBuffers;


            fastgltf::Asset asset;
            fastgltf::Parser parser{};

            auto load = parser.loadGltfBinary(buf, path.parent_path(), options);

            if (!load) {
                Logger::warn("GLTF file not found");
                return std::nullopt;
            }

            asset = std::move(load.get());
            Vector<SharedPtr<VulkanMeshAsset>> meshes;

            Vector<uint32_t> indices;
            Vector<Vertex> vertices;

            for (auto& mesh: asset.meshes) {
                VulkanMeshAsset meshAsset;
                meshAsset.name = mesh.name;

                indices.clear();
                vertices.clear();

                for (auto&& prim: mesh.primitives) {
                    VulkanMeshGeoSurface surface;
                    surface.beginIndex = indices.size();
                    surface.indexCount = asset.accessors[prim.indicesAccessor.value()].count;

                    size_t initVertex = vertices.size();

                    {
                        fastgltf::Accessor& indexAccessor = asset.accessors[prim.indicesAccessor.value()];
                        indices.reserve(indexAccessor.count + indices.size());
                        fastgltf::iterateAccessor<uint32_t>(
                                asset, indexAccessor,
                                [&](auto&& index) {
                                    indices.push_back(static_cast<uint32_t>(index));
                                });
                    }

                    {
                        fastgltf::Accessor& vertexAccessor = asset.accessors[prim.findAttribute("POSITION")->accessorIndex];
                        vertices.resize(vertexAccessor.count + vertices.size());
                        fastgltf::iterateAccessorWithIndex<glm::vec3>(
                                asset, vertexAccessor,
                                [&](auto&& vertex, size_t index) {
                                    Vertex vtx;
                                    vtx.pos = vertex;
                                    vtx.normal = {1.0f, 0.0f, 0.0f};
                                    vtx.color = glm::vec4(1.0f);
                                    vtx.uv_x = 0;
                                    vtx.uv_y = 0;

                                    vertices[index + initVertex] = vtx;
                                });
                    }

                    auto normals = prim.findAttribute("NORMAL");
                    if (normals != prim.attributes.end()) {
                        fastgltf::iterateAccessorWithIndex<glm::vec3>(
                                asset, asset.accessors[normals->accessorIndex],
                                [&](auto&& normal, size_t index) {
                                    vertices[index + initVertex].normal = normal;
                                });
                    }

                    auto uv = prim.findAttribute("TEXCOORD_0");
                    if (uv != prim.attributes.end()) {
                        fastgltf::iterateAccessorWithIndex<glm::vec2>(
                                asset, asset.accessors[uv->accessorIndex],
                                [&](auto&& texCoord, size_t index) {
                                    vertices[index + initVertex].uv_x = texCoord.x;
                                    vertices[index + initVertex].uv_y = texCoord.y;
                                });
                    }

                    auto color = prim.findAttribute("COLOR_0");
                    if (color != prim.attributes.end()) {
                        fastgltf::iterateAccessorWithIndex<glm::vec4>(
                                asset, asset.accessors[color->accessorIndex],
                                [&](auto&& col, size_t index) {
                                    vertices[index + initVertex].color = col;
                                });
                    }

                    meshAsset.surfaces.push_back(surface);
                }

                meshAsset.gpuBuffer = engine.uploadMesh(indices, vertices);
                meshes.emplace_back(std::make_shared<VulkanMeshAsset>(std::move(meshAsset)));
            }

            return VulkanMeshAssetGroup{std::move(meshes)};
        }
    }// namespace VkLoaders
}// namespace moe