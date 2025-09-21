#ifndef MOE_MATERIAL_GLSL
#define MOE_MATERIAL_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

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

#endif// MOE_MATERIAL_GLSL