#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "sampler.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragColor;

layout(push_constant, scalar) uniform SkyBox_PCS {
    mat4 inverseViewProj;
    vec4 cameraPos;
    uint skyboxImageIndex;
}
u_skyBoxPushConstants;

void main() {
    vec2 uv = inUV;
    vec2 ndc = uv * 2.0 - vec2(1.0);

    vec4 worldPos = u_skyBoxPushConstants.inverseViewProj * vec4(ndc, 1.0, 1.0);
    vec3 samplePos = normalize(worldPos.xyz / worldPos.w - u_skyBoxPushConstants.cameraPos.xyz);

    outFragColor = sampleTextureCubeLinear(u_skyBoxPushConstants.skyboxImageIndex, samplePos);
}