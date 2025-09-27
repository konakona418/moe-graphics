#ifndef MOE_SAMPLER_GLSL
#define MOE_SAMPLER_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform texture2D u_textures[];
layout(set = 0, binding = 0) uniform textureCube u_textureCubes[];
layout(set = 0, binding = 0) uniform texture2DArray u_textureArrays[];

layout(set = 0, binding = 1) uniform sampler u_samplers[];

#define SAMPLER_NEAREST_ID 0
#define SAMPLER_LINEAR_ID 1

#ifndef UINT_MAX
#define UINT_MAX 0xffffffff
#endif

#define INVALID_TEXTURE_ID UINT_MAX

vec4 sampleTextureLinear(uint index, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(u_textures[index], u_samplers[SAMPLER_LINEAR_ID])), uv);
}

vec4 sampleTextureNearest(uint index, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(u_textures[index], u_samplers[SAMPLER_NEAREST_ID])), uv);
}

vec2 textureSize2D(uint index) {
    return textureSize(nonuniformEXT(sampler2D(u_textures[index], u_samplers[SAMPLER_NEAREST_ID])), 0);
}

vec4 sampleTextureCubeLinear(uint index, vec3 direction) {
    return texture(nonuniformEXT(samplerCube(u_textureCubes[index], u_samplers[SAMPLER_LINEAR_ID])), direction);
}

vec4 sampleTextureCubeNearest(uint index, vec3 direction) {
    return texture(nonuniformEXT(samplerCube(u_textureCubes[index], u_samplers[SAMPLER_NEAREST_ID])), direction);
}

vec4 sampleTextureArrayLinear(uint index, vec2 uv, uint layer) {
    return texture(nonuniformEXT(sampler2DArray(u_textureArrays[index], u_samplers[SAMPLER_LINEAR_ID])), vec3(uv, layer));
}

vec4 sampleTextureArrayNearest(uint index, vec2 uv, uint layer) {
    return texture(nonuniformEXT(sampler2DArray(u_textureArrays[index], u_samplers[SAMPLER_NEAREST_ID])), vec3(uv, layer));
}

#endif// MOE_SAMPLER_GLSL