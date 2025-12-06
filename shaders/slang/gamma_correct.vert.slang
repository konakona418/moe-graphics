struct VertexOutput {
    float2 uv : TEXCOORD0;
    float4 position : SV_Position;
};

[[shader("vertex")]]
VertexOutput main(uint vertexIndex: SV_VulkanVertexID) {
    // Use Vulkan vertex ID.
    // SV_VertexID and SV_VulkanVertexId is not the same
    // the latter is compatible with Vulkan's vertex indexing, and is equivalent to gl_VertexIndex in GLSL
    // while the former requires some vk11 features
    VertexOutput output;
    output.uv = float2((vertexIndex << 1) & 2, vertexIndex & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}
