// [moe("vertex", "fragment")]

import moe.vertex;

struct CsmDepthPCS {
    float4x4 mvp;
    VertexBuffer vertexBuffer;
};

[vk::push_constant]
CsmDepthPCS pcs;


[shader("vertex")]
float4 vertexMain(uint vertexIndex: SV_VulkanVertexID) : SV_Position {
    Vertex inVertex = pcs.vertexBuffer[vertexIndex];
    return mul(pcs.mvp, float4(inVertex.position, 1.0));
}


[shader("fragment")]
void fragmentMain() {}
