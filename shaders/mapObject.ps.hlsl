
[[vk::binding(2, PER_PASS)]] SamplerState _sampler;
[[vk::binding(3, PER_PASS)]] ByteAddressBuffer _materialData;
[[vk::binding(4, PER_PASS)]] Texture2D<float4> _textures[1024];

struct MaterialParam
{
    uint materialID;
};

[[vk::push_constant]] MaterialParam materialParam;

struct Material
{
    uint textureIDs[3];
    float alphaTestVal;
    uint materialType;
};

struct PSInput
{
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

Material LoadMaterial()
{
    Material material;

    material = _materialData.Load<Material>(materialParam.materialID * 20); // 20 = sizeof(Material)

    return material;
}

float3 Lighting(float3 color)
{
    return float3(0, 0, 0);
}

PSOutput main(PSInput input)
{
    PSOutput output;

    Material material = LoadMaterial();

    float4 color0 = _textures[material.textureIDs[0]].Sample(_sampler, input.uv0);
    float4 color1 = _textures[material.textureIDs[1]].Sample(_sampler, input.uv1);

    if (color0.a < material.alphaTestVal)
    {
        discard;
    }

    //output.color = color0;

    if (material.materialType == 3) // Environment
    {
        float3 color = color0.rgb * color1.rgb;
        output.color = float4(Lighting(color0.rgb) + color, 1.0f);
    }
    else if (material.materialType == 5) // Environment metal
    {
        float3 color = color0.rgb * color1.rgb * color0.a;
        output.color = float4(Lighting(color0.rgb) + color, 1.0f);
    }
    else if (material.materialType == 6) // Two Layer Diffuse
    {
        static const float4 vertexcolor = float4(0, 0, 0, 0); // TODO: Load vertex colors

        float3 color = lerp(color0.rgb, color1.rgb, 1.0f);
        output.color = float4(lerp(color, color0.rgb, vertexcolor.a), 1.0f); // TODO: Apply lighting
    }
    else // Default blending, could be Diffuse, Specular, Metal or Opaque
    {
        output.color = float4(color0.rgb, 1.0f); // TODO: Apply lighting
    }

    /*if (material.materialType == 0)
    {
        output.color = float4(1, 0, 0, 1);
    }
    else if (material.materialType == 1)
    {
        output.color = float4(0, 1, 0, 1);
    }
    else
    {
        output.color = float4(0, 0, 1, 1);
    }*/

    return output;
}