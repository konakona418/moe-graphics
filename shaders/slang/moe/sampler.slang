#pragma warning(disable: 39001)
// supress warning 39001 about texture arrays using the same binding

[[vk::binding(0, 0)]]
Texture2D u_textures[];
[[vk::binding(0, 0)]]
TextureCube u_textureCubes[];
[[vk::binding(0, 0)]]
Texture2DArray u_textureArrays[];

[[vk::binding(1, 0)]]
SamplerState u_samplers[];

static const uint SAMPLER_NEAREST_ID = 0;
static const uint SAMPLER_LINEAR_ID = 1;

static const uint UINT_MAX = 0xffffffff;
static const uint INVALID_TEXTURE_ID = UINT_MAX;

float4 sampleTextureLinear(uint index, float2 uv) {
    Texture2D t = u_textures[NonUniformResourceIndex(index)];
    SamplerState s = u_samplers[SAMPLER_LINEAR_ID];
    return t.Sample(s, uv);
}

float4 sampleTextureNearest(uint index, float2 uv) {
    Texture2D t = u_textures[NonUniformResourceIndex(index)];
    SamplerState s = u_samplers[SAMPLER_NEAREST_ID];
    return t.Sample(s, uv);
}

float2 textureSize2D(uint index) {
    Texture2D t = u_textures[NonUniformResourceIndex(index)];
    uint width, height;
    t.GetDimensions(width, height);
    return float2(width, height);
}

float4 sampleTextureCubeLinear(uint index, float3 direction) {
    TextureCube t = u_textureCubes[NonUniformResourceIndex(index)];
    SamplerState s = u_samplers[SAMPLER_LINEAR_ID];
    return t.Sample(s, direction);
}

float4 sampleTextureCubeNearest(uint index, float3 direction) {
    TextureCube t = u_textureCubes[NonUniformResourceIndex(index)];
    SamplerState s = u_samplers[SAMPLER_NEAREST_ID];
    return t.Sample(s, direction);
}

float4 sampleTextureArrayLinear(uint index, float2 uv, uint layer) {
    Texture2DArray t = u_textureArrays[NonUniformResourceIndex(index)];
    SamplerState s = u_samplers[SAMPLER_LINEAR_ID];
    return t.Sample(s, float3(uv, (float) layer));
}

float4 sampleTextureArrayNearest(uint index, float2 uv, uint layer) {
    Texture2DArray t = u_textureArrays[NonUniformResourceIndex(index)];
    SamplerState s = u_samplers[SAMPLER_NEAREST_ID];
    return t.Sample(s, float3(uv, (float) layer));
}
