#version 450

#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"

layout(push_constant) uniform ShadowMap_PCS {
    mat4 mvp;
    VertexBuffer vertexBuffer;
}
u_shadowMapPushConstants;

void main() {
    Vertex inVertex = u_shadowMapPushConstants.vertexBuffer.vertices[gl_VertexIndex];
    gl_Position = u_shadowMapPushConstants.mvp * vec4(inVertex.position, 1.0);
}