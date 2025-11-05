#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "pcs.glsl"

layout(location = 0) in OutVertex inVertex;

layout(location = 0) out vec4 outFragColor;

void main() {
    vec4 outColor = inVertex.color;
    float pointSize = inVertex.size;
    float smoothing = smoothstep(0.5, 0.5 - (AA_FACTOR / pointSize), length(gl_PointCoord - vec2(0.5)));
    outColor.a *= smoothing;
    outFragColor = outColor;
}