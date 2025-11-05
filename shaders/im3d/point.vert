#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "pcs.glsl"

layout(location = 0) out OutVertex outVertex;

void main() {
    Im3dVertex inVertex = u_pushConstants.vertexBuffer.vertices[gl_VertexIndex];
    vec4 color = unpackColorLittleEndian(inVertex.color);
    color.a *= smoothstep(0.0, 1.0, inVertex.positionSize.w / AA_FACTOR);
    outVertex.color = color;
    outVertex.size = max(inVertex.positionSize.w, AA_FACTOR);
    gl_Position = u_pushConstants.viewProj * vec4(inVertex.positionSize.xyz, 1.0);
    gl_PointSize = outVertex.size;
}