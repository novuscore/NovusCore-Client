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
        u32 id;
        u32 name;
        u32 internalName;
        u32 instanceType;
        u32 flags;
        u32 expansion;
        u32 maxPlayers;
    };

    // Some of these flags were named by Nix based on WowDev.Wiki and marked with "Investigate" comment
    struct AreaTableFlag
    {
        u32 enableBreathParticles : 1;
        u32 breathParticlesOverrideParent : 1;
        u32 unk_0x4 : 1;
        u32 slaveCaptial : 1; // Investigate this flag
        u32 unk_0x10 : 1;
        u32 slaveCaptial2 : 1; // Investigate this flag
        u32 duelingEnabled : 1;
        u32 isArena : 1;
        u32 isMainCaptial : 1;
        u32 linkedChatArea : 1;
        u32 isOutland : 1; // Investigate this flag
        u32 isSanctuary : 1;
        u32 needFlyingToReach : 1;
        u32 unused_0x2000 : 1;
        u32 isNorthrend : 1; // Investigate this flag
        u32 isSubZonePVP : 1; // Investigate this flag
        u32 isInstancedArena : 1;
        u32 unused_0x20000 : 1;
        u32 isContestedArea : 1;
        u32 isDKStartingArea : 1; // Investigate this flag
        u32 isStarterZone : 1; // Investigate this flag (Supposedly enabled for areas where area level is under 15
        u32 isTown : 1;
        u32 unk_0x400000 : 1;
        u32 unk_0x800000 : 1;
        u32 unk_0x1000000 : 1; // Investigate this flag (Related to Wintergrasp)
        u32 isInside : 1; // Investigate this flag
        u32 isOutside : 1; // Investigate this flag
        u32 unk_0x8000000 : 1; // Investigate this flag (Related to Wintergrasp)
        u32 disallowFlying : 1;
        u32 useParentForWorldDefenseVisibilityCheck : 1;
    };

    struct AreaTable
    {
        u32 id;
        u32 mapId;
        u32 parentId; // Sub Areas refer to their parent areas
        u32 areaBit;
        AreaTableFlag flags;
        u32 areaLevel;
        u32 name;
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