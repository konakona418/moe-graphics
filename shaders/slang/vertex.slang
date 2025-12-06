struct Vertex {
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 tangent;
}

typedef Ptr<Vertex, Access.Read> VertexBuffer;
typedef Ptr<Vertex, Access.ReadWrite> OutputVertexBuffer;
