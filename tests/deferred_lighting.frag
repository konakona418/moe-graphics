#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : require

#include "pbr_lighting.glsl"
#include "sampler.glsl"
#include "scene_data.glsl"

layout(location = 0) in vec2 inUV;

layout(push_constant, scalar) uniform DeferredLighting_PCS {
    SceneDataBuffer sceneData;
    uint gDepthTexture;
    uint gAlbedoTexture;
    uint gNormalTexture;
    uint gORMATexture;
    uint gEmissiveTexture;
}
u_deferredPCS;

layout(location = 0) out vec4 outFragColor;

vec3 fromUVDepthToWorld(vec2 uv, float depth, mat4 invProj, mat4 invView) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = invProj * clip;
    view /= view.w;
    vec4 world = invView * view;
    return world.xyz;
}

void main() {
    vec2 uv = inUV;

    vec4 depth = sampleTextureLinear(u_deferredPCS.gDepthTexture, uv);
    vec3 albedo = sampleTextureLinear(u_deferredPCS.gAlbedoTexture, uv).xyz;
    vec3 normal = sampleTextureLinear(u_deferredPCS.gNormalTexture, uv).xyz;
    vec4 orma = sampleTextureLinear(u_deferredPCS.gORMATexture, uv);
    vec3 emissive = sampleTextureLinear(u_deferredPCS.gEmissiveTexture, uv).xyz;

    float occlusion = orma.x;
    float roughness = orma.y;
    float metallic = orma.z;
    float ambientOcclusion = orma.w;

    // ! fixme: this is extremely slow
    mat4 invProj = inverse(u_deferredPCS.sceneData.projection);
    mat4 invView = inverse(u_deferredPCS.sceneData.view);

    vec3 dialectricSpecular = vec3(0.04);
    vec3 diffuseColor = mix(albedo * (1.0 - dialectricSpecular.r), albedo, metallic);
    vec3 f0 = mix(dialectricSpecular, albedo, metallic);

    vec3 cameraPos = u_deferredPCS.sceneData.cameraPosition.xyz;
    vec3 fragPos = fromUVDepthToWorld(uv, depth.r, invProj, invView);
    vec3 v = normalize(cameraPos - fragPos);
    vec3 n = normal;

    vec3 fragColor = vec3(0.0);

    for (int i = 0; i < u_deferredPCS.sceneData.numLights; i++) {
        Light light = u_deferredPCS.sceneData.lightBuffer.lights[i];

        // for directional lights
        // this revert operation is to match the definition
        // that l is from the fragment to the light
        // ! todo: check if this is correct
        vec3 l = normalize(-light.direction);
        if (light.type != LIGHT_TYPE_DIRECTIONAL) {
            l = normalize(light.position - fragPos);
        }
        float NoL = clamp(dot(n, l), 0.0, 1.0);

        fragColor += calculateLight(light, fragPos, n, v, l, diffuseColor, roughness, metallic, f0, occlusion);
    }

    fragColor += emissive;

    // todo: ambient intensity not defined, use alpha
    fragColor += albedo * u_deferredPCS.sceneData.ambientColor.rgb * u_deferredPCS.sceneData.ambientColor.a;

    outFragColor = vec4(fragColor, 1.0);
}