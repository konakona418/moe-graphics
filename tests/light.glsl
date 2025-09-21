#ifndef MOE_LIGHT_GLSL
#define MOE_LIGHT_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

#extension GL_GOOGLE_include_directive : require

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_SPOT 2

struct Light {
    vec3 position;// 12
    uint type;    // 4

    vec3 color;     // 12
    float intensity;// 4

    float radius;  // 4
    uint available;// 4
    uint _padding1;// 4
    uint _padding2;// 4
};

layout(buffer_reference, std430) readonly buffer LightBuffer {
    Light lights[];
};

#endif// MOE_LIGHT_GLSL