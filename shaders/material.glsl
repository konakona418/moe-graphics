#ifndef MOE_MATERIAL_GLSL
#define MOE_MATERIAL_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

struct Material {
    vec4 baseColor;
    vec4 metallicRoughnessEmissive;
    vec4 emissiveColor;

    uint diffuseImageIndex;
    uint normalImageIndex;
    uint metallicRoughnessImageIndex;
    uint emissiveImageIndex;
};

layout(buffer_reference, scalar) readonly buffer MaterialBuffer {
    Material materials[];
};

#endif// MOE_MATERIAL_GLSL