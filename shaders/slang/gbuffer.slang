// [moe("vertex", "fragment")]

import moe.scene_data;
import moe.vertex;
import moe.common;
import moe.material;
import moe.sampler;

struct MeshPCS {
    float4x4 transform;
    float3x3 clampedInverseTransform;
    VertexBuffer vertexBuffer;
    SceneDataBuffer sceneData;
    MaterialId materialIndex;
}

struct VertexOutput {
    float4 position : SV_Position;
    float3 outNormal;
    float3 outColor;
    float2 outUV;
    float3 outTangent;
    float3 outWorldPos;
    float3x3 outTBN;
};

[vk::push_constant]
MeshPCS pcs;

[shader("vertex")]
VertexOutput vertexMain(uint vertexIndex: SV_VulkanVertexID) {
    VertexOutput output;

    float4x4 transform = pcs.transform;
    float3x3 inverseTransform = pcs.clampedInverseTransform;
    float4x4 viewProjection = pcs.sceneData.viewProjection;

    Vertex inVertex = pcs.vertexBuffer[vertexIndex];

    float4x4 mvp = mul(viewProjection, transform);
    output.position = mul(mvp, float4(inVertex.position, 1.0));

    output.outNormal = mul(transpose(inverseTransform), inVertex.normal);
    output.outColor = float3(1.0, 1.0, 1.0);
    output.outUV = float2(inVertex.uv_x, inVertex.uv_y);
    output.outTangent = inVertex.tangent.xyz;
    output.outWorldPos = mul(transform, float4(inVertex.position, 1.0)).xyz;

    float3 T = normalize(mul(transform, float4(inVertex.tangent.xyz, 0.0)).xyz);
    float3 N = normalize(output.outNormal);
    float3 B = cross(N, T) * inVertex.tangent.w;
    output.outTBN = float3x3(T, B, N);

    return output;
}

static const float SAMPLE_ALPHA_DISCARD_THRESHOLD = 0.5;

[shader("fragment")]
float4[4] fragmentMain(VertexOutput input) : SV_Target<4> {
    float2 uv = input.outUV;

    Material material = pcs.sceneData.materialBuffer[pcs.materialIndex];

    float4 sampledDiffuse = sampleTextureLinear(material.diffuseImageIndex, uv);
    float4 sampledMetallicRoughness = sampleTextureLinear(material.metallicRoughnessImageIndex, uv);
    float4 sampledNormal = sampleTextureLinear(material.normalImageIndex, uv);
    float4 sampledEmissive = sampleTextureLinear(material.emissiveImageIndex, uv);

    float3 baseColor = material.baseColor.rgb * sampledDiffuse.rgb;

    // ! fixme: this is too naive, need to do proper alpha blending
    // ! however, G-buffer, by nature, does not support blending well
    // ! but i don't want to combine forward pipeline and deferred either
    // !
    // ! found on complex models, e.g. exported from vroid studio
    // ! which blends multiple layers for facial expressions
    if (material.baseColor.a * sampledDiffuse.a < SAMPLE_ALPHA_DISCARD_THRESHOLD) {
        discard;
    }

    float3 normal = normalize(input.outNormal).rgb;
    if (all(input.outTangent != float3(0.0, 0.0, 0.0))) {
        normal = sampledNormal.rgb;
        normal = mul(input.outTBN, normalize(normal * 2.0 - 1.0));
        normal = normalize(normal);
    }

    float metallicF = material.metallicRoughnessEmissive.x;
    float roughnessF = material.metallicRoughnessEmissive.y;
    float emissiveF = material.metallicRoughnessEmissive.z;

    float metallic = metallicF * sampledMetallicRoughness.b;
    float roughness = roughnessF * sampledMetallicRoughness.g;
    roughness = clamp(roughness, 0.01, 1.0);
    float3 emissive = emissiveF * sampledEmissive.rgb * material.emissiveColor.rgb;

    float occlusion = 1.0;

    // todo: occlusion, AO
    float ambientOcclusion = occlusion;

    return float4[4](
            float4(baseColor, 1.0),
            float4(normal, 1.0),
            float4(occlusion, roughness, metallic, ambientOcclusion),
            float4(emissive, 1.0));
}
