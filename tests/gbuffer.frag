#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable

#define USE_SHADOW_MAP

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

#ifdef USE_SHADOW_MAP
#define USE_CSM
#ifndef USE_CSM
float sampleShadow(mat4 lightViewProj, vec3 worldPos, uint shadowMapId) {
    vec4 lightSpacePos = lightViewProj * vec4(worldPos, 1.0);
    lightSpacePos /= lightSpacePos.w;

    vec2 ndc = lightSpacePos.xy;
    ndc = ndc * 0.5 + 0.5;

    if (ndc.x < 0.0 || ndc.x > 1.0 || ndc.y < 0.0 || ndc.y > 1.0) return 0.0;

    float closestDepth = sampleTextureLinear(shadowMapId, ndc).r;
    float currentDepth = lightSpacePos.z;

    float bias = 0.001;
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}

#else

uint getCascadeIndex(vec3 worldPos, vec3 cameraPos, vec4 cascadeSplits) {
    float d = distance(worldPos.xyz, cameraPos.xyz);
    // ! usually this should be the projection on the xOz plane,
    // ! but it seems to break things, so just use the distance anyway
    for (uint i = 0; i < CASCADED_SHADOW_MAPS - 1; i++) {
        if (d < cascadeSplits[i]) {
            return i;
        }
    }
    return CASCADED_SHADOW_MAPS - 1;
}

float sampleShadow(vec3 worldPos, uint shadowMapId) {
    vec3 cameraPos = u_meshPushConstants.sceneData.cameraPosition.xyz;
    vec4 cascadeSplits = u_meshPushConstants.sceneData.shadowMapCascadeSplits;

    uint cascadeIdx = getCascadeIndex(worldPos, cameraPos, cascadeSplits);

    mat4 lightViewProj = u_meshPushConstants.sceneData.shadowMapLightTransform[cascadeIdx];
    vec4 lightSpacePos = lightViewProj * vec4(worldPos, 1.0);
    lightSpacePos /= lightSpacePos.w;

    vec2 ndc = lightSpacePos.xy;
    ndc = ndc * 0.5 + 0.5;

    if (ndc.x < 0.0 || ndc.x > 1.0 || ndc.y < 0.0 || ndc.y > 1.0) return 0.0;

    float closestDepth = sampleTextureArrayLinear(shadowMapId, ndc, cascadeIdx).r;
    float currentDepth = lightSpacePos.z;

    float bias = 0.001;
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}
#endif
#endif// USE_SHADOW_MAP

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

#ifdef USE_SHADOW_MAP
#ifndef USE_CSM
    float shadow = sampleShadow(
            u_meshPushConstants.sceneData.shadowMapLightTransform,
            inWorldPos,
            u_meshPushConstants.sceneData.shadowMapId);
#else
    float shadow = sampleShadow(inWorldPos, u_meshPushConstants.sceneData.shadowMapId);
#endif
    float occlusion = 1.0 - shadow;
#else
    float occlusion = 1.0;
#endif// USE_SHADOW_MAP

    // todo: occlusion, AO
    float ambientOcclusion = occlusion;

    outFragAlbedo = vec4(baseColor, 1.0);
    outFragNormal = vec4(normal, 1.0);
    outFragORMA = vec4(occlusion, roughness, metallic, ambientOcclusion);
    outFragEmissive = vec4(emissive, 1.0);
}