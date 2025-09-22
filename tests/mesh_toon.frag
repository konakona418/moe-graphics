#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable

#include "mesh_pcs.glsl"
#include "pbr_lighting.glsl"
#include "sampler.glsl"


layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inWorldPos;
layout(location = 5) in mat3 inTBN;

layout(location = 0) out vec4 outFragColor;

float luminance(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec2 uv = inUV;

    Material material = u_meshPushConstants.sceneData.materialBuffer.materials[u_meshPushConstants.materialIndex];

    vec4 sampledDiffuse = sampleTextureLinear(material.diffuseImageIndex, uv);
    vec4 sampledMetallicRoughness = sampleTextureLinear(material.metallicRoughnessImageIndex, uv);
    vec4 sampledNormal = sampleTextureLinear(material.normalImageIndex, uv);
    vec4 sampledEmissive = sampleTextureLinear(material.emissiveImageIndex, uv);

    vec3 baseColor = material.baseColor.rgb * sampledDiffuse.rgb;

    /*if (material.baseColor.a < 0.5) {
        discard;
    }*/

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

    vec3 dialectricSpecular = vec3(0.04);
    vec3 black = vec3(0.0);
    vec3 diffuseColor = mix(baseColor * (1.0 - dialectricSpecular.r), baseColor, metallic);
    vec3 f0 = mix(dialectricSpecular, baseColor, metallic);

    vec3 cameraPos = u_meshPushConstants.sceneData.cameraPosition.xyz;
    vec3 fragPos = inWorldPos.xyz;
    vec3 v = normalize(cameraPos - fragPos);
    vec3 n = normal;

    vec3 fragColor = vec3(0.0);

    for (int i = 0; i < u_meshPushConstants.sceneData.numLights; i++) {
        Light light = u_meshPushConstants.sceneData.lightBuffer.lights[i];

        // for directional lights
        // this revert operation is to match the definition
        // that l is from the fragment to the light
        // ! todo: check if this is correct
        vec3 l = normalize(-light.direction);
        if (light.type != LIGHT_TYPE_DIRECTIONAL) {
            l = normalize(light.position - fragPos);
        }
        float NoL = clamp(dot(n, l), 0.0, 1.0);

        // todo: occlusion
        float occlusion = 1.0;

        fragColor += calculateLight(light, fragPos, n, v, l, diffuseColor, roughness, metallic, f0, occlusion);
    }

    vec3 emissive = emissiveF * sampledEmissive.rgb * material.emissiveColor.rgb;
    fragColor += emissive;

    // todo: ambient intensity not defined, use alpha
    fragColor += baseColor * u_meshPushConstants.sceneData.ambientColor.rgb * u_meshPushConstants.sceneData.ambientColor.a;

    float intensity = luminance(fragColor);

    float toonLevel = 0.0;
    if (intensity > 0.8)
        toonLevel = 1.0;
    else if (intensity > 0.4)
        toonLevel = 0.7;
    else if (intensity > 0.1)
        toonLevel = 0.4;
    else
        toonLevel = 0.1;

    vec3 toonColor = fragColor * toonLevel;

    toonColor += emissive;
    toonColor += baseColor * u_meshPushConstants.sceneData.ambientColor.rgb * u_meshPushConstants.sceneData.ambientColor.a;

    outFragColor = vec4(toonColor, 1.0);
}