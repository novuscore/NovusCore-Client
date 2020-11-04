#include "globalData.inc.hlsl"

[[vk::binding(3, PER_PASS)]] SamplerState _sampler;
[[vk::binding(4, PER_PASS)]] ByteAddressBuffer _materialParams;
[[vk::binding(5, PER_PASS)]] ByteAddressBuffer _materialData;
[[vk::binding(6, PER_PASS)]] Texture2D<float4> _textures[4096];

struct MaterialParam
{
    uint16_t materialID;
    uint16_t exteriorLit;
};

struct Material
{
    uint16_t textureIDs[3];
    uint16_t alphaTestVal;
    uint16_t materialType;
    uint16_t isUnlit;
};

struct PSInput
{
    float3 normal : TEXCOORD0;
    float4 color0 : TEXCOORD1;
    float4 color1 : TEXCOORD2;
    float4 uv01 : TEXCOORD3;
    uint materialParamID : TEXCOORD4;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

MaterialParam LoadMaterialParam(uint materialParamID)
{
    MaterialParam materialParam;
    
    materialParam = _materialParams.Load<MaterialParam>(materialParamID * 4); // 4 = sizeof(MaterialParam)
    
    return materialParam;
}

Material LoadMaterial(uint materialID)
{
    Material material;

    material = _materialData.Load<Material>(materialID * 12); // 12 = sizeof(Material)

    return material;
}

float3 Lighting(float3 baseColor, float4 vertexColor, float3 normal, Material material, MaterialParam materialParam)
{
    float3 currColor;
    float3 lDiffuse = float3(0, 0, 0);
    float3 accumlatedLight = float3(1.0f, 1.0f, 1.0f);

    if (materialParam.exteriorLit == 1)
    {
        float nDotL = saturate(dot(normal, -normalize(_lightData.lightDir.xyz)));

        float3 ambientColor = _lightData.ambientColor.rgb + vertexColor.rgb;

        float3 skyColor = (ambientColor * 1.10000002);
        float3 groundColor = (ambientColor * 0.699999988);

        currColor = lerp(groundColor, skyColor, 0.5 + (0.5 * nDotL));
        lDiffuse = _lightData.lightColor.rgb * nDotL;
    }
    else
    {
        currColor = _lightData.ambientColor.rgb + vertexColor.rgb;
    }

    float3 gammaDiffTerm = baseColor * (currColor + lDiffuse);
    float3 linearDiffTerm = (baseColor * baseColor) * accumlatedLight;
    return sqrt(gammaDiffTerm * gammaDiffTerm + linearDiffTerm);
}

PSOutput main(PSInput input)
{
    PSOutput output;
    
    MaterialParam materialParam = LoadMaterialParam(input.materialParamID);
    Material material = LoadMaterial(materialParam.materialID);
    
    // Doing this, I get errors only when GPU validation is enabled
    //float4 tex0 = _textures[material.textureIDs[0]].Sample(_sampler, input.uv01.xy);
    //float4 tex1 = _textures[material.textureIDs[1]].Sample(_sampler, input.uv01.zw);
    
    // If I do this instead, it works
    uint textureID0 = material.textureIDs[0]; // Prevents invalid patching of shader when running GPU validation layers, maybe remove in future
    uint textureID1 = material.textureIDs[1]; // Prevents invalid patching of shader when running GPU validation layers, maybe remove in future
    float4 tex0 = _textures[textureID0].Sample(_sampler, input.uv01.xy);
    float4 tex1 = _textures[textureID1].Sample(_sampler, input.uv01.zw);

    float alphaTestVal = f16tof32(material.alphaTestVal);
    if (tex0.a < alphaTestVal)
    {
        discard;
    }
    
    float3 normal = normalize(input.normal);
    
    if (material.materialType == 0) // Diffuse
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material, materialParam), input.color0.a);
    }
    else if (material.materialType == 1) // Specular
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material, materialParam), input.color0.a);
    }
    else if (material.materialType == 2) // Metal
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material, materialParam), input.color0.a);
    }
    else if (material.materialType == 3) // Environment
    {
        float3 matDiffuse = tex0.rgb;
        float3 env = tex1.rgb * tex0.a;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material, materialParam) + env, input.color0.a);
    }
    else if (material.materialType == 4) // Opaque
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0, normal, material, materialParam), input.color0.a);
    }
    else if (material.materialType == 5) // Environment metal
    {
        float3 matDiffuse = tex0.rgb;
        float3 env = (tex0.rgb * tex0.a) * tex1.rgb;
        output.color = float4(Lighting(tex0.rgb, input.color0, normal, material, materialParam) + env, input.color0.a);
    }
    else if (material.materialType == 6) // Two Layer Diffuse
    {
        float3 layer0 = tex0.rgb;
        float3 layer1 = lerp(layer0, tex1.rgb, tex1.a);
        float3 matDiffuse = (input.color0.rgb * 2.0) * lerp(layer1, layer0, input.color1.a);

        output.color = float4(Lighting(matDiffuse, input.color0, normal, material, materialParam), 1.0f);
    }
    
    return output;
}