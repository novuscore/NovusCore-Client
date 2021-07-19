#include "globalData.inc.hlsl"

struct SkybandColors
{
    float4 top;
    float4 middle;
    float4 bottom;
    float4 aboveHorizon;
    float4 horizon;
};
[[vk::push_constant]] SkybandColors _skybandColors;

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float Map(float value, float originalMin, float originalMax, float newMin, float newMax)
{
    return (value - originalMin) / (originalMax - originalMin) * (newMax - newMin) + newMin;
}

float4 main(VertexOutput input) : SV_Target
{
    float3 rotation = _viewData.eyeRotation.xyz;

    float resolutionY = 1080.0f;
    float fovY = 75.0f;
    float halfFovY = fovY / 2.0f;

    float uvRotationOffset = ((1.0f - input.uv.y) * fovY) - halfFovY;
    float val = (rotation.y + uvRotationOffset + 89.0f) / 178.0f;
    val = clamp(val, 0.0f, 1.0f);

    if (val < 0.50f)
    {
        return _skybandColors.horizon;
    }
    else if (val < 0.515f)
    {
        float blendFactor = Map(val, 0.50f, 0.515f, 0.0f, 1.0f);
        return lerp(_skybandColors.horizon, _skybandColors.aboveHorizon, blendFactor);
    }
    else if (val < 0.60f)
    {
        float blendFactor = Map(val, 0.515f, 0.60f, 0.0f, 1.0f);
        return lerp(_skybandColors.aboveHorizon, _skybandColors.bottom, blendFactor);
    }
    else if (val < 0.75f)
    {
        float blendFactor = Map(val, 0.60f, 0.75f, 0.0f, 1.0f);
        return lerp(_skybandColors.bottom, _skybandColors.middle, blendFactor);
    }

    float blendFactor = Map(val, 0.75f, 1.0f, 0.0f, 1.0f);
    return lerp(_skybandColors.middle, _skybandColors.top, blendFactor);
}