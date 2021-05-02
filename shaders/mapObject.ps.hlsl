permutation COLOR_PASS = [0, 1];

#include "common.inc.hlsl"
#include "globalData.inc.hlsl"
#include "mapObject.inc.hlsl"

struct PackedMaterialParam
{
    uint packed; // uint16_t materialID, uint16_t exteriorLit
}; // 4 bytes

struct MaterialParam
{
    uint materialID;
    uint exteriorLit;
};

struct PackedMaterial
{
    uint packed0; // uint16_t textureID0, uint16_t textureID1
    uint packed1; // uint16_t textureID2, uint16_t alphaTestVal
    uint packed2; // uint16_t materialType, uint16_t isUnlit
}; // 12 bytes

struct Material
{
    uint textureIDs[3];
    uint alphaTestVal;
    uint materialType;
    uint isUnlit;
};

[[vk::binding(3, PER_PASS)]] SamplerState _sampler;
[[vk::binding(4, PER_PASS)]] StructuredBuffer<PackedMaterialParam> _packedMaterialParams;
[[vk::binding(5, PER_PASS)]] StructuredBuffer<PackedMaterial> _packedMaterialData;
[[vk::binding(6, PER_PASS)]] Texture2D<float4> _ambientOcclusion;
[[vk::binding(7, PER_PASS)]] Texture2D<float4> _textures[4096];

struct PSInput
{
    float4 position : SV_Position;
    uint materialParamID : TEXCOORD0;
    float4 uv01 : TEXCOORD1;
#if COLOR_PASS
    float3 normal : TEXCOORD2;
    float4 color0 : TEXCOORD3;
    float4 color1 : TEXCOORD4;
    uint instanceLookupID : TEXCOORD5;
#endif
};

struct PSOutput
{
#if COLOR_PASS
    float4 color : SV_Target0;
    uint objectID : SV_Target1;
#endif
};

MaterialParam LoadMaterialParam(uint materialParamID)
{
    PackedMaterialParam packedMaterialParam = _packedMaterialParams[materialParamID];
    
    MaterialParam materialParam;
    
    materialParam.materialID = packedMaterialParam.packed & 0xFFFF;
    materialParam.exteriorLit = (packedMaterialParam.packed >> 16) & 0xFFFF;
    
    return materialParam;
}

Material LoadMaterial(uint materialID)
{
    PackedMaterial packedMaterial = _packedMaterialData[materialID];
    
    Material material;

    material.textureIDs[0] = packedMaterial.packed0 & 0xFFFF;
    material.textureIDs[1] = (packedMaterial.packed0 >> 16) & 0xFFFF;
    material.textureIDs[2] = packedMaterial.packed1 & 0xFFFF;
    material.alphaTestVal = (packedMaterial.packed1 >> 16) & 0xFFFF;
    material.materialType = packedMaterial.packed2 & 0xFFFF;
    material.isUnlit = (packedMaterial.packed2 >> 16) & 0xFFFF;

    return material;
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
    float4 tex0 = _textures[NonUniformResourceIndex(textureID0)].Sample(_sampler, input.uv01.xy);

    float alphaTestVal = f16tof32(material.alphaTestVal);
    if (tex0.a < alphaTestVal)
    {
        discard;
    }

#if COLOR_PASS
    uint textureID1 = material.textureIDs[1]; // Prevents invalid patching of shader when running GPU validation layers, maybe remove in future
    float4 tex1 = _textures[NonUniformResourceIndex(textureID1)].Sample(_sampler, input.uv01.zw);
    
    float3 normal = normalize(input.normal);
    bool isLit = materialParam.exteriorLit == 1;
    float ambientOcclusion = _ambientOcclusion.Load(float3(input.position.xy, 0)).x;
    
    if (material.materialType == 0) // Diffuse
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0.rgb, normal, ambientOcclusion, isLit), input.color0.a);
    }
    else if (material.materialType == 1) // Specular
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0.rgb, normal, ambientOcclusion, isLit), input.color0.a);
    }
    else if (material.materialType == 2) // Metal
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0.rgb, normal, ambientOcclusion, isLit), input.color0.a);
    }
    else if (material.materialType == 3) // Environment
    {
        float3 matDiffuse = tex0.rgb;
        float3 env = tex1.rgb * tex0.a;
        output.color = float4(Lighting(matDiffuse, input.color0.rgb, normal, ambientOcclusion, isLit) + env, input.color0.a);
    }
    else if (material.materialType == 4) // Opaque
    {
        float3 matDiffuse = tex0.rgb;
        output.color = float4(Lighting(matDiffuse, input.color0.rgb, normal, ambientOcclusion, isLit), input.color0.a);
    }
    else if (material.materialType == 5) // Environment metal
    {
        float3 matDiffuse = tex0.rgb;
        float3 env = (tex0.rgb * tex0.a) * tex1.rgb;
        output.color = float4(Lighting(tex0.rgb, input.color0.rgb, normal, ambientOcclusion, isLit) + env, input.color0.a);
    }
    else if (material.materialType == 6) // Two Layer Diffuse
    {
        float3 layer0 = tex0.rgb;
        float3 layer1 = lerp(layer0, tex1.rgb, tex1.a);
        float3 matDiffuse = (input.color0.rgb * 2.0) * lerp(layer1, layer0, input.color1.a);

        output.color = float4(Lighting(matDiffuse, input.color0.rgb, normal, ambientOcclusion, isLit), 1.0f);
    }

    // 4 most significant bits are used as a type identifier, remaining bits are instanceLookupID
    output.objectID = uint(ObjectType::MapObject) << 28;
    output.objectID += input.instanceLookupID;
#endif

    return output;
}