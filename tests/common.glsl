#ifndef COMMON_GLSL
#define COMMON_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

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
    vec4 basColor;
    vec4 metalicRoughnessEmmissive;

    uint diffuseImageIndex;
    uint normalImageIndex;
    uint metallicRoughnessImageIndex;
    uint emmissiveImageIndex;
};

layout(buffer_reference, std430) readonly buffer MaterialBuffer {
    Material materials[];
};

layout(buffer_reference, std430) readonly buffer SceneDataBuffer {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;

    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;

    //MaterialBuffer materials;
};

layout(push_constant, scalar) uniform PCS {
    mat4 transform;
    VertexBuffer vertexBuffer;
    SceneDataBuffer sceneData;
    uint materialIndex;
}
PushConstants;

#endif
