// [moe("vertex", "fragment")]

import moe.pbr_lighting;
import moe.sampler;
import moe.common;
import moe.light;
import moe.scene_data;

struct VertexOutput {
    float2 uv;
    float4 position : SV_Position;
}

VertexOutput vertexMain(uint vertexIndex: SV_VulkanVertexID) {
    VertexOutput output;
    output.uv = float2((vertexIndex << 1) & 2, vertexIndex & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

struct DeferredLightingPCS {
    SceneDataBuffer sceneData;
    ImageId gDepthTexture;
    ImageId gAlbedoTexture;
    ImageId gNormalTexture;
    ImageId gORMATexture;
    ImageId gEmissiveTexture;
}

[vk::push_constant]
DeferredLightingPCS pcs;

uint getCascadeIndex(float3 worldPos, float3 cameraPos, float4 cascadeSplits) {
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

float sampleShadow(float3 worldPos, uint shadowMapId, float NoL) {
    float3 cameraPos = pcs.sceneData.cameraPosition.xyz;
    float4 cascadeSplits = pcs.sceneData.shadowMapCascadeSplits;

    uint cascadeIdx = getCascadeIndex(worldPos, cameraPos, cascadeSplits);

    float4x4 lightViewProj = pcs.sceneData.shadowMapLightTransform[cascadeIdx];
    float4 lightSpacePos = mul(lightViewProj, float4(worldPos, 1.0));
    lightSpacePos /= lightSpacePos.w;

    float2 ndc = lightSpacePos.xy;
    ndc = ndc * 0.5 + 0.5;

    if (ndc.x < 0.0 || ndc.x > 1.0 || ndc.y < 0.0 || ndc.y > 1.0) return 0.0;

    // dynamic bias
    const float constBias = 0.00001;
    const float slopeFactor = 0.0005;

    float slopeBias = slopeFactor * (1.0 - abs(NoL));

    float bias = constBias + slopeBias;

    float shadow = 0.0;
    const int numSamples = 8;

    float2 poissonDisk[8] = float2[](
            float2(-0.326, -0.406), float2(0.374, 0.415),
            float2(0.175, -0.312), float2(-0.357, 0.283),
            float2(-0.011, -0.439), float2(0.439, -0.139),
            float2(0.141, 0.141), float2(-0.141, -0.141));

    float2 texelSize = 1.0 / textureSize2D(pcs.sceneData.shadowMapId);

    // ! todo: add a falloff to sample radius if needed
    float radius = 2.5;

    for (int i = 0; i < numSamples; i++) {
        float2 offset = poissonDisk[i] * texelSize * radius;
        float2 sampleNDC = ndc + offset;
        float closestDepth = sampleTextureArrayLinear(shadowMapId, sampleNDC, cascadeIdx).r;
        float currentDepth = lightSpacePos.z;

        shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
    }
    shadow /= float(numSamples);
    return shadow;
}

float3 fromUVDepthToWorld(float2 uv, float depth, float4x4 invProj, float4x4 invView) {
    // float4 clip = float4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    // ! the depth is already in clip space
    float4 clip = float4(uv * 2.0 - 1.0, depth, 1.0);
    float4 view = mul(invProj, clip);
    view /= view.w;
    float4 world = mul(invView, view);
    return world.xyz;
}

float4 sampleSkyBox(float2 uv) {
    uint skyboxId = pcs.sceneData.skyboxId;
    if (skyboxId == INVALID_TEXTURE_ID) {
        return float4(0.0);
    }

    float2 ndc = uv * 2.0 - float2(1.0);

    float4 worldPos = mul(pcs.sceneData.invViewProjection, float4(ndc, 1.0, 1.0));
    float3 samplePos = normalize(worldPos.xyz / worldPos.w - pcs.sceneData.cameraPosition.xyz);

    return sampleTextureCubeLinear(skyboxId, samplePos);
}

bool screenSpaceShadowTestSpot(float3 fragPosWorld, float3 lightPosWorld, float4x4 view, float4x4 proj, uint depthTexture) {
    // world to view space
    float3 fragPos = (mul(view, float4(fragPosWorld, 1.0))).xyz;
    float3 lightPos = (mul(view, float4(lightPosWorld, 1.0))).xyz;

    float3 L = normalize(lightPos - fragPos);
    float distanceToLight = length(lightPos - fragPos);

    const int maxSteps = 16;
    const float bias = 0.01;
    for (int i = 0; i < maxSteps; i++) {
        float t = float(i) / float(maxSteps);
        float3 samplePos = fragPos + L * distanceToLight * t;

        float4 clip = mul(proj, float4(samplePos, 1.0));
        float3 ndc = clip.xyz / clip.w;
        float2 sampleUV = ndc.xy * 0.5 + 0.5;

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

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target {
    float2 uv = input.uv;

    float4 depth = sampleTextureLinear(pcs.gDepthTexture, uv);
    float3 albedo = sampleTextureLinear(pcs.gAlbedoTexture, uv).xyz;
    float3 normal = sampleTextureLinear(pcs.gNormalTexture, uv).xyz;
    float4 orma = sampleTextureLinear(pcs.gORMATexture, uv);
    float3 emissive = sampleTextureLinear(pcs.gEmissiveTexture, uv).xyz;

    if (depth.r == 1.0) {
        // no geometry, sample skybox
        return sampleSkyBox(uv);
    }

    float occlusion = orma.x;
    float roughness = orma.y;
    float metallic = orma.z;
    float ambientOcclusion = orma.w;

    float4x4 invView = pcs.sceneData.invView;
    float4x4 invProj = pcs.sceneData.invProjection;

    float3 dialectricSpecular = float3(0.04);
    float3 diffuseColor = lerp(albedo * (1.0 - dialectricSpecular.r), albedo, metallic);
    float3 f0 = lerp(dialectricSpecular, albedo, metallic);

    float3 cameraPos = pcs.sceneData.cameraPosition.xyz;
    float3 fragPos = fromUVDepthToWorld(uv, depth.r, invProj, invView);
    float3 v = normalize(cameraPos - fragPos);
    float3 n = normal;

    float3 fragColor = float3(0.0);

    for (int i = 0; i < pcs.sceneData.numLights; i++) {
        Light light = pcs.sceneData.lightBuffer[i];

        bool inShadow = false;

        // for directional lights
        // this revert operation is to match the definition
        // that l is from the fragment to the light
        // ! todo: check if this is correct
        float3 l = normalize(-light.direction);
        if (light.type != LightType.Directional) {
            l = normalize(light.position - fragPos);
        }
        float NoL = clamp(dot(n, l), 0.0, 1.0);

        // ! fixme: this is just a hack
        const float noOcclusion = 1.0;
        if (light.type == LightType.Directional) {
            // directional light, use CSM occlusion
            float shadow = sampleShadow(fragPos, pcs.sceneData.shadowMapId, NoL);
            float occlusion = 1.0 - shadow;

            occlusion = clamp(occlusion, 0.1, 1.0);// clamp to avoid black
            fragColor += calculateLight(light, fragPos, n, v, l, diffuseColor, roughness, metallic, f0, occlusion);
        } else {
            // other, use no occlusion
            fragColor += calculateLight(light, fragPos, n, v, l, diffuseColor, roughness, metallic, f0, noOcclusion);
        }
    }

    fragColor += emissive;

    // todo: ambient intensity not defined, use alpha
    fragColor += albedo * pcs.sceneData.ambientColor.rgb * pcs.sceneData.ambientColor.a;

    return float4(fragColor, 1.0);
}
