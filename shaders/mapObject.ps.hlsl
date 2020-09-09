#include "globalData.inc.hlsl"

[[vk::binding(1, PER_PASS)]] SamplerState _sampler;
[[vk::binding(2, PER_PASS)]] ByteAddressBuffer _materialData;
[[vk::binding(3, PER_PASS)]] Texture2D<float4> _textures[1024];

struct MaterialParam
{
    uint materialID;
    uint exteriorLit;
};

[[vk::push_constant]] MaterialParam materialParam;

struct Material
{
    uint textureIDs[3];
    float alphaTestVal;
    uint materialType;
    uint isUnlit;
};

struct PSInput
{
    float3 normal : TEXCOORD0;
    float4 color0 : TEXCOORD1;
    float4 color1 : TEXCOORD2;
    float4 uv01 : TEXCOORD3;
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

float3 Lighting(float3 baseColor, float4 vertexColor, float3 normal, Material material)
{
    float3 currColor;
    float3 lDiffuse = float3(0, 0, 0);

    if (materialParam.exteriorLit == 1)
    {
        float nDotL = dot(normal, -normalize(_lightData.lightDir.xyz));

        float3 ambientColor = _lightData.ambientColor.rgb + vertexColor.rgb;

        float3 skyColor = (ambientColor * 1.10000002);
        float3 groundColor = (ambientColor * 0.699999988);

        currColor = lerp(groundColor, skyColor, 0.5 + (0.5 * nDotL));
        lDiffuse = _lightData.lightColor.rgb * saturate(nDotL);
    }
    else
    {
        currColor = _lightData.ambientColor.rgb + vertexColor.rgb;
    }

    float3 gammaDiffTerm = baseColor * (currColor + lDiffuse);
    return gammaDiffTerm;
}

PSOutput main(PSInput input)
{
    PSOutput output;

    Material material = LoadMaterial();

    float4 tex0 = _textures[material.textureIDs[0]].Sample(_sampler, input.uv01.xy);
    float4 tex1 = _textures[material.textureIDs[1]].Sample(_sampler, input.uv01.zw);

    if (tex0.a < material.alphaTestVal)
    {
        discard;
    }

    float3 normal = normalize(input.normal);

    if (material.materialType == 0) // Diffuse
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material), input.color0.a);
    }
    else if (material.materialType == 1) // Specular
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material), input.color0.a);
    }
    else if (material.materialType == 2) // Metal
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material), input.color0.a);
    }
    else if (material.materialType == 3) // Environment
    {
        float3 matDiffuse = tex0.rgb;
        float3 env = tex1.rgb * tex0.a;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material) + env, input.color0.a);
    }
    else if (material.materialType == 4) // Opaque
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material), input.color0.a);
    }
    else if (material.materialType == 5) // Environment metal
    {
        float3 matDiffuse = tex0.rgb;
        float3 env = (tex0.rgb * tex0.a) * tex1.rgb;
        output.color = float4(Lighting(tex0.rgb, input.color0, normal, material) + env, input.color0.a);
    }
    else if (material.materialType == 6) // Two Layer Diffuse
    {
        float3 layer0 = tex0.rgb;
        float3 layer1 = lerp(layer0, tex1.rgb, tex1.a);
        float3 matDiffuse = (input.color0.rgb * 2.0) * lerp(layer1, layer0, input.color1.a);

        output.color = float4(Lighting(matDiffuse, input.color0, normal, material), 1.0f);
    }

    return output;
}