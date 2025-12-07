// [moe("vertex", "fragment")]

import moe.sampler;
import moe.common;

struct FxaaPCS {
    uint inputImageId;
    float2 screenSize;
    float2 inverseScreenSize;
}

struct VertexOutput {
    float2 uv;
    float2 posNW;
    float2 posNE;
    float2 posSW;
    float2 posSE;
    float2 posM;
    float4 vertexPosition : SV_Position;
}

[vk::push_constant]
FxaaPCS fxaaPCS;

void texcoords(float2 fragCoord, float2 resolutionInv,
               out float2 posNW, out float2 posNE,
               out float2 posSW, out float2 posSE,
               out float2 posM) {
    float2 inverseVP = resolutionInv;
    posNW = (fragCoord + float2(-1.0, -1.0)) * inverseVP;
    posNE = (fragCoord + float2(1.0, -1.0)) * inverseVP;
    posSW = (fragCoord + float2(-1.0, 1.0)) * inverseVP;
    posSE = (fragCoord + float2(1.0, 1.0)) * inverseVP;
    posM = float2(fragCoord * inverseVP);
}

[shader("vertex")]
VertexOutput vertexMain(uint vertexIndex: SV_VulkanVertexID) {
    VertexOutput output;
    output.uv = float2((vertexIndex << 1) & 2, vertexIndex & 2);
    texcoords(output.uv * fxaaPCS.screenSize,
              fxaaPCS.inverseScreenSize,
              output.posNW, output.posNE,
              output.posSW, output.posSE,
              output.posM);
    output.vertexPosition = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

/*
Based on:
    - https://github.com/mattdesl/glsl-fxaa/blob/master/fxaa.glsl by Matt DesLauriers
        Open source under the MIT License
*/

static const float FXAA_REDUCE_MIN = (1.0 / 128.0);
static const float FXAA_REDUCE_MUL = (1.0 / 8.0);
static const float FXAA_SPAN_MAX = 8.0;

float4 fxaa(uint tex, float2 fragCoord, float2 resolutionInv,
            float2 rgbNW, float2 rgbNE,
            float2 rgbSW, float2 rgbSE,
            float2 rgbM) {
    float4 color;
    float2 inverseVP = resolutionInv;
    float3 rgbNW = sampleTextureLinear(tex, rgbNW).xyz;
    float3 rgbNE = sampleTextureLinear(tex, rgbNE).xyz;
    float3 rgbSW = sampleTextureLinear(tex, rgbSW).xyz;
    float3 rgbSE = sampleTextureLinear(tex, rgbSE).xyz;
    float4 texColor = sampleTextureLinear(tex, rgbM);
    float3 rgbM = texColor.xyz;
    float3 luma = float3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(rgbM, luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                                  (0.25 * FXAA_REDUCE_MUL),
                          FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(float2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
                  dir * rcpDirMin)) *
          inverseVP;

    float3 rgbA = 0.5 * (sampleTextureLinear(tex, fragCoord * inverseVP + dir * (1.0 / 3.0 - 0.5)).xyz +
                         sampleTextureLinear(tex, fragCoord * inverseVP + dir * (2.0 / 3.0 - 0.5)).xyz);
    float3 rgbB = rgbA * 0.5 + 0.25 * (sampleTextureLinear(tex, fragCoord * inverseVP + dir * -0.5).xyz +
                                       sampleTextureLinear(tex, fragCoord * inverseVP + dir * 0.5).xyz);

    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        color = float4(rgbA, texColor.a);
    else
        color = float4(rgbB, texColor.a);
    return color;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput vertexOut) {
    return fxaa(fxaaPCS.inputImageId,
                vertexOut.uv * fxaaPCS.screenSize.xy,
                fxaaPCS.inverseScreenSize,
                vertexOut.posNW,
                vertexOut.posNE,
                vertexOut.posSW,
                vertexOut.posSE,
                vertexOut.posM);
}
