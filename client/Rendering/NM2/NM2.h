#pragma once
#include <NovusTypes.h>
#include <vector>

#pragma pack(push, 1)
namespace NM2
{
    struct NM2Header
    {
        u32 token = 0;
        u32 version = 0;
    };

    struct M2Vertex
    {
        vec3 position = vec3(0, 0, 0);
        u8 boneWeights[4] = { 0, 0, 0, 0 };
        u8 boneIndices[4] = { 0, 0, 0, 0 };
        vec3 normal = vec3(0, 0, 0);
        vec2 uvCords[2] = { vec2(0, 0), vec2(0, 0) };
    };

    enum class TextureTypes : u32
    {
        DEFAULT,
        SKIN,
        OBJECT_SKIN,
        WEAPON_BLADE,
        WEAPON_HANDLE,
        ENVIRONMENT, // OBSOLETE according to https://wowdev.wiki/M2#Textures
        CHARACTER_HAIR,
        CHARACTER_FACIAL_HAIR, // OBSOLETE according to https://wowdev.wiki/M2#Textures
        SKIN_EXTRA,
        UI_SKIN,
        TAUREN_MANE, // OBSOLETE according to https://wowdev.wiki/M2#Textures
        MONSTER_SKIN_1,
        MONSTER_SKIN_2,
        MONSTER_SKIN_3,
        ITEM_ICON
    };

    struct M2Texture
    {
        TextureTypes type = TextureTypes::DEFAULT; // Check https://wowdev.wiki/M2#Textures
        struct M2TextureFlags
        {
            u32 wrapX : 1;
            u32 wrapY : 1;
        } flags;

        u32 textureNameIndex = 0;
    };

    struct M2Material
    {
        struct M2MaterialFlags
        {
            u16 unLit : 1;
            u16 unFogged : 1;
            u16 disableBackfaceCulling : 1;
            u16 depthTest;
            u16 depthWrite;
            u16 : 5;
        } flags;
        u16 blendingMode; // Check https://wowdev.wiki/M2/Rendering#M2BLEND
    };

    struct M2Skin
    {
        u32 token;

        std::vector<u16> vertexIndexes;
        std::vector<u16> indices;
    };

    struct NM2Root
    {
        NM2Header header;

        std::vector<M2Vertex> vertices;
        std::vector<M2Texture> textures;
        std::vector<M2Material> materials;
        std::vector<u16> textureIndicesToId;
        std::vector<u16> textureCombos;
        M2Skin skin;
    };
}
#pragma pack(pop)