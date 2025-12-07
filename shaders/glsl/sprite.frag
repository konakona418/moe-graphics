#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout : require

#include "sampler.glsl"

layout(push_constant, scalar) uniform PCS_Transform {
    mat4 transform;

    vec4 color;
    vec2 spriteSize;
    vec2 texRegionOffset;
    vec2 texRegionSize;
    vec2 texSize;
    uint texId;
    uint isTextSprite;
}
u_pushConstants;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outFragColor;

void main() {
    vec2 uv = inUV;

    vec4 outColor;
    if (u_pushConstants.texId != INVALID_TEXTURE_ID) {
        if (u_pushConstants.isTextSprite != 0) {
            outColor = u_pushConstants.color;
            outColor.a *= sampleTextureNearest(u_pushConstants.texId, uv).r;
        } else {
            outColor =
                    sampleTextureNearest(
                            u_pushConstants.texId,
                            uv) *
                    u_pushConstants.color;
        }
    } else {
        outColor = u_pushConstants.color;
    }

    if (outColor.a < 0.01) {
        discard;
    }

    outFragColor = outColor;
}