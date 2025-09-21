#ifndef MOE_SAMPLER_GLSL
#define MOE_SAMPLER_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform texture2D u_textures[];
layout(set = 0, binding = 1) uniform sampler u_samplers[];

#define SAMPLER_NEAREST_ID 0
#define SAMPLER_LINEAR_ID 1

vec4 sampleTextureLinear(uint index, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(u_textures[index], u_samplers[SAMPLER_LINEAR_ID])), uv);
}

vec4 sampleTextureNearest(uint index, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(u_textures[index], u_samplers[SAMPLER_NEAREST_ID])), uv);
}

#endif// MOE_SAMPLER_GLSL