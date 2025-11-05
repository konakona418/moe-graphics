#version 450

#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"

layout(push_constant) uniform CSM_PCS {
    mat4 mvp;
    VertexBuffer vertexBuffer;
}
u_csmPushConstants;

void main() {
    Vertex inVertex = u_csmPushConstants.vertexBuffer.vertices[gl_VertexIndex];
    gl_Position = u_csmPushConstants.mvp * vec4(inVertex.position, 1.0);
}