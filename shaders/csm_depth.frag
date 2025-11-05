#version 450

#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"

layout(push_constant) uniform CSM_PCS {
    mat4 mvp;
    VertexBuffer vertexBuffer;
}
u_csmPushConstants;

void main() {
    // nothing to do here
}