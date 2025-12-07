#version 450

#include "sampler.glsl"

layout(location = 0) in vec2 inUV;

layout(push_constant, scalar) uniform GammaCorrect_PCS {
    uint inputTexture;
    float gamma;
}
u_gammaPCS;

layout(location = 0) out vec4 outFragColor;

void main() {
    vec4 color = sampleTextureLinear(u_gammaPCS.inputTexture, inUV);
    outFragColor = pow(color, vec4(1.0 / u_gammaPCS.gamma));
}