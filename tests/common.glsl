#ifndef COMMON_GLSL
#define COMMON_GLSL

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

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

struct Material {
    vec4 baseColor;
    vec4 metalicRoughnessEmmissive;

    uint diffuseImageIndex;
    uint normalImageIndex;
    uint metallicRoughnessImageIndex;
    uint emmissiveImageIndex;
};

layout(buffer_reference, std430) readonly buffer MaterialBuffer {
    Material materials[];
};

layout(buffer_reference, scalar) readonly buffer SceneDataBuffer {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;

    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;

    MaterialBuffer materialBuffer;
};

layout(push_constant, scalar) uniform PCS {
    mat4 transform;
    VertexBuffer vertexBuffer;
    SceneDataBuffer sceneData;
    uint materialIndex;
}
PushConstants;

#endif
