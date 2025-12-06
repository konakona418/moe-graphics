import vertex;

struct SkinningData {
    uint4 jointIndices;
    float4 jointWeights;
};

struct SkinningPCS {
    VertexBuffer vertexBuffer;
    StructuredBuffer<SkinningData> skinningDataBuffer;
    StructuredBuffer<float4x4> jointMatrices;
    OutputVertexBuffer outputVertexBuffer;
    uint jointMatrixStartIndex;
    uint vertexCount;
};

[[vk::push_constant]]
cbuffer SkinningPC : register(b0) {
    SkinningPCS pcs;
};

float4x4 getJointMatrix(uint jointIndex) {
    return pcs.jointMatrices[pcs.jointMatrixStartIndex + jointIndex];
}

[numthreads(128, 1, 1)]
void main(
    uint3 dispatchThreadID : SV_DispatchThreadID
) {
    uint idx = dispatchThreadID.x;
    if (idx >= pcs.vertexCount) {
        return;
    }

    SkinningData skinData = pcs.skinningDataBuffer[idx];
    float4x4 skinMatrix = 
        getJointMatrix(skinData.jointIndices.x) * skinData.jointWeights.x +
        getJointMatrix(skinData.jointIndices.y) * skinData.jointWeights.y +
        getJointMatrix(skinData.jointIndices.z) * skinData.jointWeights.z +
        getJointMatrix(skinData.jointIndices.w) * skinData.jointWeights.w;

    Vertex inVertex = pcs.vertexBuffer[idx];
    Vertex outVertex;
    outVertex.position = mul(skinMatrix, float4(inVertex.position, 1.0)).xyz;

    float3x3 normalMatrix = (float3x3)skinMatrix;
    outVertex.normal = normalize(mul(normalMatrix, inVertex.normal));

    outVertex.tangent = float4(normalize(mul(normalMatrix, inVertex.tangent.xyz)), inVertex.tangent.w);

    outVertex.uv_x = inVertex.uv_x;
    outVertex.uv_y = inVertex.uv_y;

    pcs.outputVertexBuffer[idx] = outVertex;
}