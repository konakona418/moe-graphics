#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout : require

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

// 4 indices - rect
vec3 indexToVertexUVMapper(uint idx) {
    vec2 uv = vec2((idx == 1 || idx == 2) ? 1.0 : 0.0,
                   (idx == 2 || idx == 3) ? 1.0 : 0.0);
    return vec3(uv, 0.0);
}

layout(location = 0) out vec2 outUV;

void main() {
    vec3 localUV = indexToVertexUVMapper(gl_VertexIndex);

    vec2 uvMin = u_pushConstants.texRegionOffset / u_pushConstants.texSize;
    vec2 uvMax = (u_pushConstants.texRegionOffset + u_pushConstants.texRegionSize) / u_pushConstants.texSize;

    outUV = mix(uvMin, uvMax, localUV.xy);

    // by default, the sprite origin is at the top-left corner
    gl_Position = u_pushConstants.transform *
                  vec4(vec3(localUV.xy * u_pushConstants.spriteSize, localUV.z), 1.0);
}