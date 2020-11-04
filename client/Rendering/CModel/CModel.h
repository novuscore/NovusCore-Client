#pragma once
#include <NovusTypes.h>
#include <NovusTypeHeader.h>
#include <vector>

#pragma pack(push, 1)
namespace CModel
{
    struct ComplexVertex
    {
        hvec3 position = hvec3(f16(0), f16(0), f16(0));
        u8 octNormal[2] = { 0 };
        hvec2 uvCords[2] = { hvec2(f16(0), f16(0)), hvec2(f16(0), f16(0)) };
    };

    enum class ComplexTextureType : u32
    {
        NONE = 0,
        COMPONENT_SKIN,
        COMPONENT_OBJECT_SKIN,
        COMPONENT_WEAPON_BLADE,
        COMPONENT_WEAPON_HANDLE,
        COMPONENT_ENVIRONMENT, // OBSOLETE
        COMPONENT_CHAR_HAIR,
        COMPONENT_CHAR_FACIAL_HAIR, // OBSOLETE
        COMPONENT_UI_SKIN,
        COMPONENT_TAUREN_MANE, // OBSOLETE
        COMPONENT_MONSTER_SKIN_1,
        COMPONENT_MONSTER_SKIN_2,
        COMPONENT_MONSTER_SKIN_3,
        COMPONENT_ITEM_ICON
    };

    struct ComplexTextureFlag
    {
        u32 wrapX : 1;
        u32 wrapY : 1;
    };

    struct ComplexTexture
    {
        ComplexTextureType type = ComplexTextureType::NONE; // Check https://wowdev.wiki/M2#Textures
        ComplexTextureFlag flags;

        u32 textureNameIndex;
    };

    struct ComplexMaterialFlag
    {
        u16 unLit : 1;
        u16 unFogged : 1;
        u16 disableBackfaceCulling : 1;
        u16 depthTest : 1;
        u16 depthWrite : 1;
        u16 Unk_1 : 1;
        u16 Unk_2 : 1;
        u16 Unk_3 : 1;
        u16 Unk_4 : 1;
        u16 Unk_5 : 1;
    };

    struct ComplexMaterial
    {
        ComplexMaterialFlag flags;
        u16 blendingMode; // Check https://wowdev.wiki/M2/Rendering#M2BLEND
    };

    // Check https://wowdev.wiki/M2/.skin#Texture_units
    enum class ComplexTextureUnitFlag : u8
    {
        ANIMATED = 0,
        INVERTED_MATERIALS = 1,
        TRANSFORM = 2,
        PROJECTED_TEXTURE = 4,
        STATIC_TEXTURE = 16,
    };

    struct ComplexTextureUnit
    {
        ComplexTextureUnitFlag flags;

        u16 shaderId;
        u16 materialIndex;
        u16 materialLayer;

        u16 textureCount;
        u16 textureIndices[2] = { 0, 0 };
        u16 textureUVAnimationIndices[2] = { 0, 0 };
        u16 textureUnitLookupId;
        u16 textureTransparencyLookupId;

        // TODO: Add the remaining data later
    };

    struct ComplexRenderBatch
    {
        u16 groupId = 0;
        u32 vertexStart = 0;
        u32 vertexCount = 0;
        u32 indexStart = 0;
        u32 indexCount = 0;

        // TODO: Add the remaining data later

        std::vector<ComplexTextureUnit> textureUnits;
    };

    struct ComplexModelData
    {
        NovusTypeHeader header = NovusTypeHeader(11, 1);

        std::vector<u16> vertexLookupIds;
        std::vector<u16> indices; // These are relative to the index of vertexLookupIds and needs to be translated
        std::vector<ComplexRenderBatch> renderBatches;

        // TODO: Add the remaining data later
    };

    struct ComplexModelFlag
    {
        u32 Tilt_X : 1;
        u32 Tilt_Y : 1;
        u32 : 1;
        u32 Use_Texture_Combiner_Combos : 1; // (TBC+)
        u32 : 1; // (TBC+)
        u32 Load_Physics_Data : 1; // (MOP+)
        u32 : 1; // (MOP+)
        u32 Unk_0x80 : 1; // (WOD+)
        u32 Camera_Related : 1; // (WOD+)
        u32 New_Particle_Record : 1; // (Legion+)
        u32 Unk_0x400 : 1; // (Legion+)
        u32 Texture_Transforms_Use_Bone_Sequences : 1; // (Legion+)

        // 0x1000 to 0x200000 are unk (Legion+)
    };

    struct ComplexModel
    {
    public:
        NovusTypeHeader header = NovusTypeHeader(10, 1);

        char* name;
        ComplexModelFlag flags;
        ComplexModelData modelData;

        std::vector<ComplexVertex> vertices;
        std::vector<ComplexTexture> textures;
        std::vector<ComplexMaterial> materials;

        std::vector<u16> textureIndexLookupTable;
        std::vector<u16> textureUnitLookupTable;
        std::vector<u16> textureTransparencyLookupTable;
        std::vector<u16> textureUVAnimationLookupTable;

        std::vector<u16> textureCombinerCombos;
    };
}
#pragma pack(pop)