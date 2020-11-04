#include "globalData.inc.hlsl"
#include "cModel.inc.hlsl"

[[vk::binding(3, PER_PASS)]] ByteAddressBuffer _textureUnits;
[[vk::binding(4, PER_PASS)]] SamplerState _sampler;
[[vk::binding(5, PER_PASS)]] Texture2D<float4> _textures[4096];

struct TextureUnitIndices
{
    uint16_t packedIndices[4]; // 8 indices packed into 4 uint16s
};

[[vk::push_constant]] TextureUnitIndices textureUnitIndices;

struct TextureUnit
{
    uint data1; // (Is Projected Texture (1 bit) + Material Flag (10 bit) + Material Blending Mode (3 bit) + Unused Padding (2 bits)) + Material Type (16 bit)
    uint textureIDs[2];
    uint padding;
};

struct PSInput
{
    uint drawCallID : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 uv01 : TEXCOORD2;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

TextureUnit LoadTextureUnit(uint textureUnitIndex)
{
    TextureUnit textureUnit;
    textureUnit = _textureUnits.Load<TextureUnit>(textureUnitIndex * 16); // 16 = sizeof(TextureUnit)

    return textureUnit;
}

enum PixelShaderID
{
    Opaque,
    Opaque_Opaque,
    Opaque_Mod,
    Opaque_Mod2x,
    Opaque_Mod2xNA,
    Opaque_Add,
    Opaque_AddNA,
    Opaque_AddAlpha,
    Opaque_AddAlpha_Alpha,
    Opaque_Mod2xNA_Alpha,
    Mod,
    Mod_Opaque,
    Mod_Mod,
    Mod_Mod2x,
    Mod_Mod2xNA,
    Mod_Add,
    Mod_AddNA,
    Mod2x,
    Mod2x_Mod,
    Mod2x_Mod2x,
    Add,
    Add_Mod,
    Fade,
    Decal
};

float4 Shade(uint16_t pixelId, float4 texture1, float4 texture2, out float3 specular)
{
    float4 result = float4(0, 0, 0, 0);
    float4 diffuseColor = float4(1, 1, 1, 1);
    specular = float3(0, 0, 0);

    if (pixelId == PixelShaderID::Opaque)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb;
        result.a = diffuseColor.a;
    }
    else if (pixelId == PixelShaderID::Opaque_Opaque)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb * texture2.rgb;
        result.a = diffuseColor.a;
    }
    else if (pixelId == PixelShaderID::Opaque_Mod)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb * texture2.rgb;
        result.a = texture2.a * diffuseColor.a;
    }
    else if (pixelId == PixelShaderID::Opaque_Mod2x)
    {
        result.rgb = diffuseColor.rgb * 2.0f * texture1.rgb * texture2.rgb;
        result.a = diffuseColor.a * 2.0f * texture2.a;
    }
    else if (pixelId == PixelShaderID::Opaque_Mod2xNA)
    {
        result.rgb = diffuseColor.rgb * 2.0f * texture1.rgb * texture2.rgb;
        result.a = diffuseColor.a;
    }
    else if (pixelId == PixelShaderID::Opaque_Add)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb + texture2.rgb;
        result.a = diffuseColor.a + texture1.a;
    }
    else if (pixelId == PixelShaderID::Opaque_AddNA)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb + texture2.rgb;
        result.a = diffuseColor.a;
    }
    else if (pixelId == PixelShaderID::Opaque_AddAlpha)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb;
        result.a = diffuseColor.a;

        specular = texture2.rgb * texture2.a;
    }
    else if (pixelId == PixelShaderID::Opaque_AddAlpha_Alpha)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb;
        result.a = diffuseColor.a;

        specular = texture2.rgb * texture2.a * (1.0f - texture1.a);
    }
    else if (pixelId == PixelShaderID::Opaque_Mod2xNA_Alpha)
    {
        result.rgb = diffuseColor.rgb * lerp(texture1.rgb * texture2.rgb * 2.0f, texture1.rgb, texture1.aaa);
        result.a = diffuseColor.a;
    }
    else if (pixelId == PixelShaderID::Mod)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb;
        result.a = diffuseColor.a * texture1.a;
    }
    else if (pixelId == PixelShaderID::Mod_Opaque)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb * texture2.rgb;
        result.a = diffuseColor.a * texture1.a;
    }
    else if (pixelId == PixelShaderID::Mod_Mod)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb * texture2.rgb;
        result.a = diffuseColor.a * texture1.a * texture2.a;
    }
    else if (pixelId == PixelShaderID::Mod_Mod2x)
    {
        result.rgb = diffuseColor.rgb * 2.0f * texture1.rgb * texture2.rgb;
        result.a = diffuseColor.a * 2.0f * texture1.a * texture2.a;
    }
    else if (pixelId == PixelShaderID::Mod_Mod2xNA)
    {
        result.rgb = diffuseColor.rgb * 2.0f * texture1.rgb * texture2.rgb;
        result.a = texture1.a * diffuseColor.a;
    }
    else if (pixelId == PixelShaderID::Mod_Add)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb;
        result.a = diffuseColor.a * (texture1.a + texture2.a);
        
        specular = texture2.rgb;
    }
    else if (pixelId == PixelShaderID::Mod_AddNA)
    {
        result.rgb = diffuseColor.rgb * texture1.rgb;
        result.a = texture1.a * diffuseColor.a;

        specular = texture2.rgb;
    }
    else if (pixelId == PixelShaderID::Mod2x)
    {
        result.rgb = diffuseColor.rgb * 2.0f * texture1.rgb;
        result.a = diffuseColor.a * 2.0f * texture1.a;
    }
    else if (pixelId == PixelShaderID::Mod2x_Mod)
    {
        result.rgb = diffuseColor.rgb * 2.0f * texture1.rgb * texture2.rgb;
        result.a = diffuseColor.a * 2.0f * texture1.a * texture2.a;
    }
    else if (pixelId == PixelShaderID::Mod2x_Mod2x)
    {
        result = diffuseColor * 4.0f * texture1 * texture2;
    }
    else if (pixelId == PixelShaderID::Add)
    {
        result = diffuseColor + texture1;
    }
    else if (pixelId == PixelShaderID::Add_Mod)
    {
        result.rgb = (diffuseColor.rgb + texture1.rgb) * texture2.a;
        result.a = (diffuseColor.a + texture1.a) * texture2.a;
    }
    else if (pixelId == PixelShaderID::Fade)
    {
        result.rgb = (texture1.rgb - diffuseColor.rgb) * diffuseColor.a + diffuseColor.rgb;
        result.a = diffuseColor.a;
    }
    else if (pixelId == PixelShaderID::Decal)
    {
        result.rgb = (diffuseColor.rgb - texture1.rgb) * diffuseColor.a + texture1.rgb;
        result.a = diffuseColor.a;
    }

    result.rgb = result.rgb;
    return result;
}

float4 Blend(uint16_t blendingMode, float4 previousColor, float4 color)
{
    float4 result = previousColor;

    if (blendingMode == 0) // OPAQUE
    {
        result = float4(color.rgb, 1);
    }
    else if (blendingMode == 1) // ALPHA KEY
    {
        if (color.a >= 224.0f / 255.0f)
        {
            float3 blendedColor = color.rgb * color.a + previousColor.rgb * (1 - color.a);
            result.rgb += blendedColor;
            result.a = max(color.a, previousColor.a); // TODO: Check if this is actually needed
        }
        else
        {
            discard;
        }
    }
    else if (blendingMode == 2) // ALPHA
    {
        float3 blendedColor = color.rgb * color.a + previousColor.rgb * (1 - color.a);
        color.rgb += blendedColor;
        color.a = max(color.a, previousColor.a); // TODO: Check if this is actually needed
    }
    else if (blendingMode == 3) // NO ALPHA ADD
    {
        // TODO
        result.rgb += color.rgb;
    }
    else if (blendingMode == 4) // ADD
    {
        // TODO
        result.rgb += color.rgb * color.a;
        result.a = color.a;
    }
    else if (blendingMode == 5) // MOD
    {
        // TODO
    }
    else if (blendingMode == 6) // MOD2X
    {
        // TODO
    }
    else if (blendingMode == 7) // BLEND ADD
    {
        // TODO
    }

    return result;
}

PSOutput main(PSInput input)
{
    DrawCallData drawCallData = LoadDrawCallData(input.drawCallID);

    float4 color = float4(0, 0, 0, 0);
    float3 specular = float3(0, 0, 0);
    bool isLit = false;

    for (uint textureUnitIndex = drawCallData.textureUnitOffset; textureUnitIndex < drawCallData.textureUnitOffset + drawCallData.numTextureUnits; textureUnitIndex++)
    {
        TextureUnit textureUnit = LoadTextureUnit(textureUnitIndex);

        uint16_t isProjectedTexture = uint16_t(textureUnit.data1 & 0x1);
        uint16_t materialFlags = uint16_t((textureUnit.data1 >> 1) & 0x3FF);
        uint16_t blendingMode = uint16_t((textureUnit.data1 >> 11) & 0x7);
        
        uint16_t materialType = uint16_t((textureUnit.data1 >> 16) & 0xFFFF);
        uint16_t vertexShaderId = materialType & 0xFF;
        uint16_t pixelShaderId = materialType >> 8;

        if (materialType == 0x8000)
            continue;

        float4 texture1 = _textures[textureUnit.textureIDs[0]].Sample(_sampler, input.uv01.xy);
        float4 texture2 = float4(0, 0, 0, 0);

        if (vertexShaderId > 2)
        {
            // ENV uses generated UVCoords based on camera pos + geometry normal in frame space
            texture2 = _textures[textureUnit.textureIDs[1]].Sample(_sampler, input.uv01.zw);
        }

        isLit |= (materialFlags & 0x1);

        float4 shadedColor = Shade(pixelShaderId, texture1, texture2, specular);
        color = Blend(blendingMode, color, shadedColor);
    }

    color.rgb = Lighting(color.rgb, input.normal, !isLit) + specular;

    PSOutput output;
    output.color = saturate(color);
    return output;
}