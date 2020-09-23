#include "globalData.inc.hlsl"

[[vk::binding(1, PER_PASS)]] SamplerState _sampler;
[[vk::binding(2, PER_PASS)]] ByteAddressBuffer _materialData;
[[vk::binding(3, PER_PASS)]] Texture2D<float4> _textures[4096];

struct MaterialParam
{
    uint materialID;
};

[[vk::push_constant]] MaterialParam materialParam;

struct Material
{
    uint type;
    uint blendingMode;
    uint textureIDs[4];
};

struct PSInput
{
    float3 normal : TEXCOORD0;
    float2 uv0 : TEXCOORD1;
    float2 uv1 : TEXCOORD2;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

Material LoadMaterial()
{
    Material material;
    material = _materialData.Load<Material>(materialParam.materialID * 24); // 24 = sizeof(Material)

    return material;
}

PSOutput main(PSInput input)
{
    PSOutput output;

    Material material = LoadMaterial();

    //float4 outColor = float4(0, 0, 0, 0);
    float4 texture1 = _textures[material.textureIDs[0]].Sample(_sampler, input.uv0);


    // Apply Lighting
    float3 normal = normalize(input.normal);

    float lightFactor = max(dot(normal, -normalize(_lightData.lightDir.xyz)), 0.0);
    texture1 = texture1 * (saturate(_lightData.lightColor * lightFactor) + _lightData.ambientColor);

    output.color = texture1;
    return output;
}