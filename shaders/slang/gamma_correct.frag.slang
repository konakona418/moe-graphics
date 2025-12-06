import sampler;

struct GammaCorrectPCS {
    uint inputTextureId;
    float gamma;
}

[[vk::push_constant]]
GammaCorrectPCS pcs;

[[shader("fragment")]]
float4 main(float2 inUV: TEXCOORD0) {
    float4 color = sampleTextureLinear(pcs.inputTextureId, inUV);
    color.rgb = pow(color.rgb, float3(1.0 / pcs.gamma));
    return color;
}
