#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable

#include "mesh_pcs.glsl"
#include "sampler.glsl"

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outFragColor;

void main() {
    vec4 sampled = sampleTextureLinear(u_meshPushConstants.sceneData.materialBuffer.materials[u_meshPushConstants.materialIndex].diffuseImageIndex, vec2(inUV.x, inUV.y));
    outFragColor = sampled * vec4(inColor, 1.0);
}