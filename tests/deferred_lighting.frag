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

vec4 sampleSkyBox(vec2 uv) {
    uint skyboxId = u_deferredPCS.sceneData.skyboxId;
    if (skyboxId == INVALID_TEXTURE_ID) {
        return vec4(0.0);
    }

    vec2 ndc = uv * 2.0 - vec2(1.0);

    vec4 worldPos = u_deferredPCS.sceneData.invViewProjection * vec4(ndc, 1.0, 1.0);
    vec3 samplePos = normalize(worldPos.xyz / worldPos.w - u_deferredPCS.sceneData.cameraPosition.xyz);

    return sampleTextureCubeLinear(skyboxId, samplePos);
}

bool screenSpaceShadowTestSpot(vec3 fragPosWorld, vec3 lightPosWorld, mat4 view, mat4 proj, uint depthTexture) {
    // world to view space
    vec3 fragPos = (view * vec4(fragPosWorld, 1.0)).xyz;
    vec3 lightPos = (view * vec4(lightPosWorld, 1.0)).xyz;

    vec3 L = normalize(lightPos - fragPos);
    float distanceToLight = length(lightPos - fragPos);

    const int maxSteps = 16;
    const float bias = 0.01;
    for (int i = 0; i < maxSteps; i++) {
        float t = float(i) / float(maxSteps);
        vec3 samplePos = fragPos + L * distanceToLight * t;

        vec4 clip = proj * vec4(samplePos, 1.0);
        vec3 ndc = clip.xyz / clip.w;
        vec2 sampleUV = ndc.xy * 0.5 + 0.5;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            continue;
        }

        float sampleDepth = sampleTextureLinear(depthTexture, sampleUV).x;
        float sampleDepthLinear = ndc.z * 0.5 + 0.5;

        if (sampleDepth < sampleDepthLinear - bias) {
            return true;
        }
    }

    return false;
}

void main() {
    vec2 uv = inUV;

    vec4 depth = sampleTextureLinear(u_deferredPCS.gDepthTexture, uv);
    vec3 albedo = sampleTextureLinear(u_deferredPCS.gAlbedoTexture, uv).xyz;
    vec3 normal = sampleTextureLinear(u_deferredPCS.gNormalTexture, uv).xyz;
    vec4 orma = sampleTextureLinear(u_deferredPCS.gORMATexture, uv);
    vec3 emissive = sampleTextureLinear(u_deferredPCS.gEmissiveTexture, uv).xyz;

    if (depth.r == 1.0) {
        // no geometry, sample skybox
        outFragColor = sampleSkyBox(uv);
        return;
    }

    float occlusion = orma.x;
    float roughness = orma.y;
    float metallic = orma.z;
    float ambientOcclusion = orma.w;

    mat4 invView = u_deferredPCS.sceneData.invView;
    mat4 invProj = u_deferredPCS.sceneData.invProjection;

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

        bool inShadow = false;

#ifdef USE_SCREEN_SPACE_SHADOW
        if (light.type == LIGHT_TYPE_POINT) {
            inShadow = screenSpaceShadowTestSpot(fragPos, light.position, u_deferredPCS.sceneData.view, u_deferredPCS.sceneData.projection, u_deferredPCS.gDepthTexture);
        }

        if (inShadow) {
            continue;
        }
#endif

        // for directional lights
        // this revert operation is to match the definition
        // that l is from the fragment to the light
        // ! todo: check if this is correct
        vec3 l = normalize(-light.direction);
        if (light.type != LIGHT_TYPE_DIRECTIONAL) {
            l = normalize(light.position - fragPos);
        }
        float NoL = clamp(dot(n, l), 0.0, 1.0);

        // ! fixme: this is just a hack
        const float noOcclusion = 1.0;
        if (light.type == LIGHT_TYPE_DIRECTIONAL) {
            // directional light, use CSM occlusion
            occlusion = occlusion == 0.0 ? 0.2 : occlusion;
            fragColor += calculateLight(light, fragPos, n, v, l, diffuseColor, roughness, metallic, f0, occlusion);
        } else {
            // other, use no occlusion
            fragColor += calculateLight(light, fragPos, n, v, l, diffuseColor, roughness, metallic, f0, noOcclusion);
        }
    }

    fragColor += emissive;

    // todo: ambient intensity not defined, use alpha
    fragColor += albedo * u_deferredPCS.sceneData.ambientColor.rgb * u_deferredPCS.sceneData.ambientColor.a;

    outFragColor = vec4(fragColor, 1.0);
}