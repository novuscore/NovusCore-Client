permutation TEX_TYPE = [float, float2, float3, float4, int, int2, int3, int4, uint, uint2, uint3, uint4];

#include "common.inc.hlsl"

[[vk::binding(0, 0)]] SamplerState _sampler;
[[vk::binding(1, 0)]] Texture2D<TEX_TYPE> _texture;

struct Constants
{
    float4 colorMultiplier;
    float4 additiveColor;
};

[[vk::push_constant]] Constants _constants;

struct VSOutput
{
    float2 uv : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target
{
    float2 dimensions;
    _texture.GetDimensions(dimensions.x, dimensions.y);

    float2 uv = float2(input.uv.x, 1.0 - input.uv.y);
    int3 location = int3(uv * dimensions, 0);

    float4 color = ToFloat4(_texture.Load(location), 1.0f);
    return color * _constants.colorMultiplier + _constants.additiveColor;
}