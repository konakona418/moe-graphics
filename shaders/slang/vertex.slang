struct Vertex {
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 tangent;
};

typedef StructuredBuffer<Vertex> VertexBuffer;
typedef RWStructuredBuffer<Vertex> OutputVertexBuffer;
