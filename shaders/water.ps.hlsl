#include "globalData.inc.hlsl"

struct Constants
{
    float currentTime;
};

[[vk::push_constant]] Constants _constants;
[[vk::binding(2, PER_PASS)]] SamplerState _sampler;
[[vk::binding(3, PER_PASS)]] Texture2D<float4> _textures[128];

struct PSInput
{
    float2 uv : TEXCOORD0;
    uint textureOffset : TEXCOORD1;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    float4 color = float4(0.8314f, 0.9451f, 1.0f, 1.f);
    float4 texture1 = _textures[input.textureOffset].Sample(_sampler, input.uv);

    // Apply Lighting
    //float3 normal = float3(0.f, 1.f, 0.f);//normalize(input.normal);

    //float lightFactor = max(dot(normal, -normalize(_lightData.lightDir.xyz)), 0.0);
    //texture1 = texture1 * (saturate(_lightData.lightColor * lightFactor) + _lightData.ambientColor);

    color.rgb += texture1.rgb;
    color.a *= texture1.a;

    output.color = color;
    return output;
}