import vertex;

struct SkinningData {
    uint4 jointIndices;
    float4 jointWeights;
};

struct SkinningPCS {
    VertexBuffer vertexBuffer;
    Ptr<SkinningData, Access.Read> skinningDataBuffer;
    Ptr<float4x4, Access.Read> jointMatrices;
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
    outVertex.position = (float3)(mul(float4(inVertex.position, 1.0f), skinMatrix)).xyz;

    outVertex.normal = (float3)(mul(float4(inVertex.normal, 0.0f), skinMatrix)).xyz;

    float4 tangent_transformed = mul(float4(inVertex.tangent.xyz, 0.0f), skinMatrix);
    outVertex.tangent = float4(tangent_transformed.xyz, inVertex.tangent.w);

    outVertex.uv_x = inVertex.uv_x;
    outVertex.uv_y = inVertex.uv_y;

    pcs.outputVertexBuffer[idx] = outVertex;
}