#version 450

layout(location = 0) out vec2 outUV;

void main() {
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vec4 pos = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
    pos.z = pos.w;// hack into depth test

    gl_Position = pos;
}