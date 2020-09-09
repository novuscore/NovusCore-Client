#pragma once
#include <NovusTypes.h>
#include <Utils/ByteBuffer.h>
#include <Containers/StringTable.h>

namespace DBC
{
    constexpr i32 DBC_TOKEN = 1313096259;
    constexpr i32 DBC_VERSION = 1;

    struct DBCHeader
    {
        u32 token;
        u32 version;
    };

    struct File
    {
        DBCHeader header;
        Bytebuffer* buffer;
    };

    struct Map
    {
        u32 Id = 0;
        u32 Name = 0;
        u32 InternalName = 0;
        u32 InstanceType = 0;
        u32 Flags = 0;
        u32 Expansion = 0;
        u32 MaxPlayers = 0;
    };

    struct CreatureDisplayInfo
    {
        u32 id;
        u32 modelId; // Reference into CreatureModelData
        u32 soundId; // Reference into CreatureSoundData
        u32 extraDisplayInfoId; // Reference into CreatureDisplayInfoExtra
        f32 scale;
        u32 opacity;
        u32 texture1;
        u32 texture2;
        u32 texture3;
        u32 portraitTextureName;
        u32 bloodLevelId; // Reference into UnitBloodLevels
        u32 bloodId; // Reference into UnitBlood
        u32 npcSoundsId; // Reference into NPCSounds
        u32 particlesId; // Reference into ParticleColor
        u32 creatureGeosetData;
        u32 objectEffectPackageId; // Reference into ObjectEffectPackage
    };

    struct CreatureModelData
    {
        u32 id;
        u32 flags;
        u32 modelPath;

        u32 sizeClass;
        f32 modelScale;

        u32 bloodLevelId; // Reference into UnitBloodLevels

        u32 footPrintId; // Reference into FootprintTextures
        f32 footPrintTextureLength;
        f32 footprintTextureWidth;
        f32 footprintParticleScale;
        u32 foleyMaterialId;
        u32 footstepShakeSize; // Reference into CameraShakes
        u32 deathThudShakeSize; // Reference into CameraShakes
        u32 soundDataId; // Reference into CreatureSoundData

        f32 collisionWidth;
        f32 collisionHeight;
        f32 mountHeight;

        vec3 geoBoxMin;
        vec3 geoBoxMax;

        f32 worldEffectScale;
        f32 attachedEffectScale;

        f32 missileCollisionRadius;
        f32 missileCollisionPush;
        f32 missileCollisionRaise;
    };
}