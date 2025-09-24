#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable

#include "mesh_pcs.glsl"
#include "sampler.glsl"

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inWorldPos;
layout(location = 5) in mat3 inTBN;

// this should map that in the GBuffer pipeline attachment locations
layout(location = 0) out vec4 outFragAlbedo;
layout(location = 1) out vec4 outFragNormal;
layout(location = 2) out vec4 outFragORMA;
layout(location = 3) out vec4 outFragEmissive;

void main() {
    vec2 uv = inUV;

    Material material = u_meshPushConstants.sceneData.materialBuffer.materials[u_meshPushConstants.materialIndex];

    vec4 sampledDiffuse = sampleTextureLinear(material.diffuseImageIndex, uv);
    vec4 sampledMetallicRoughness = sampleTextureLinear(material.metallicRoughnessImageIndex, uv);
    vec4 sampledNormal = sampleTextureLinear(material.normalImageIndex, uv);
    vec4 sampledEmissive = sampleTextureLinear(material.emissiveImageIndex, uv);

    vec3 baseColor = material.baseColor.rgb * sampledDiffuse.rgb;

    vec3 normal = normalize(inNormal).rgb;
    if (inTangent != vec3(0.0)) {
        normal = sampledNormal.rgb;
        normal = inTBN * normalize(normal * 2.0 - 1.0);
        normal = normalize(normal);
    }

    float metallicF = material.metallicRoughnessEmissive.x;
    float roughnessF = material.metallicRoughnessEmissive.y;
    float emissiveF = material.metallicRoughnessEmissive.z;

    float metallic = metallicF * sampledMetallicRoughness.b;
    float roughness = roughnessF * sampledMetallicRoughness.g;
    roughness = clamp(roughness, 0.01, 1.0);
    vec3 emissive = emissiveF * sampledEmissive.rgb * material.emissiveColor.rgb;

    // todo: occlusion, AO
    float occlusion = 1.0;
    float ambientOcclusion = occlusion;

    outFragAlbedo = vec4(baseColor, 1.0);
    outFragNormal = vec4(normal, 1.0);
    outFragORMA = vec4(occlusion, roughness, metallic, ambientOcclusion);
    outFragEmissive = vec4(emissive, 1.0);
}