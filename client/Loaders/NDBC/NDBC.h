#pragma once
#include <NovusTypes.h>
#include <Utils/DynamicBytebuffer.h>
#include <Containers/StringTable.h>
#include <vector>

namespace NDBC
{
    constexpr i32 NDBC_TOKEN = 1313096259;
    constexpr i32 NDBC_VERSION = 3;

    struct NDBCColumn
    {
        std::string name;
        u32 dataType; // 0 == I32, 1 == U32, 2 == F32
    };

    struct NDBCHeader
    {
        u32 token;
        u32 version;
    };

    struct File
    {
        NDBCHeader header;

        std::vector<NDBCColumn> columns;
        u32 numRows = 0;
        size_t dataOffsetToRowData = 0;

        DynamicBytebuffer* buffer;
        StringTable* stringTable;
    };

    struct TeleportLocation
    {
        u32 id;
        u32 name;
        i32 gameVersion;
        u32 mapId;
        vec3 position;
        f32 yaw;
        f32 pitch;
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

    struct Light
    {
        u32 id;
        u32 mapId; // This is a reference to Map.dbc
        vec3 position; // X, Y and Z position for the center of the Light(Skybox) Sphere.
        vec2 fallOff; // X == Start (Inner Radius), Y == End (Outer Radius)
        i32 paramClearId; // Used in clear weather
        i32 paramClearInWaterId; // Used in clear weather while being underwater.
        i32 paramStormId; // Used in storm like weather.
        i32 paramStormInWaterId; // Used in storm like weather while being underwater.
        i32 paramDeathId; // Appears to be hardcoded in the client, but maybe not.

        // These are only used in WoTLK
        i32 paramUnk1Id;
        i32 paramUnk2Id;
        i32 paramUnk3Id;
    };

    struct LightParams
    {
        u32 id;
        u32 highlightSky;
        u32 lightSkyboxId;
        u32 cloudTypeId;
        f32 glow;
        f32 waterShallowAlpha;
        f32 waterDeepAlpha;
        f32 oceanShallowAlpha;
        f32 oceanDeepAlpha;
        u32 flags;
    };

    /*
        Each LightParams have 18 rows in LightIntBand, to get the appropriate row id do the following (LightParams->id * 18)
        The rows correspond to these values

        Number 	Description
        0 		Global ambient light
        1 		Global diffuse light
        2 		Sky color 0 (top)
        3 		Sky color 1 (middle)
        4 		Sky color 2 (middle to horizon)
        5 		Sky color 3 (above horizon)
        6 		Sky color 4 (horizon)
        7 		Fog color / background mountains color. Affects color of weather effects as well.
        8 	 	?
        9 		Sun color + sun halo color, specular lighting, sun rays
        10 		Sun larger halo color  //  cloud color a1 (base)
        11 	 	? // cloud color B (edge)
        12 		Cloud color  // cloud color a2 (secondary base)
        13 	 	?
        14 	 	Ocean color [light] // shallow ocean water
        15 		Ocean color [dark]  // deep ocean water
        16 		River color [light] // shallow river water
        17 	 	River color [dark]  // deep river water
    */
    struct LightIntBand
    {
        u32 id;
        u32 entries; // Defines how many of the columns are used in this row (0..16)

        u32 timeValues[16];
        u32 colorValues[16]; // Stores the color values for the time values (BGRX)
    };

    /*
        Each LightParams have 6 rows in LightFloatBand, to get the appropriate row id do the following (LightParams->id * 6)
        The rows correspond to these values

        Number      Description
        0           Fog distance multiplied by 36 - distance at which everything will be hidden by the fog
        1           Fog multiplier - fog distance * fog multiplier = fog start distance. 0-0,999...
        2           Celestial Glow Through - the brightness of the sun and moon as it shows through cloud cover. Note that this effect only appears when the Sun or Moon is being obscured by clouds. 0-1
        3           Cloud Density - Controls the density of cloud cover in the area. Value range is 0.0 to 1.0.
        4           ?
        5           ?
    */
    struct LightFloatBand
    {
        u32 id;
        u32 entries; // Defines how many of the columns are used in this row (0..16)

        u32 timeValues[16];
        f32 values[16];
    };

    struct LightSkybox
    {
        u32 id;
        u32 modelPath;
        u32 flags; //  0x1: animation syncs with time of day (uses animation 0, time of day is just in percentage). 0x2: render stars, sun and moons and clouds as well. 0x4: do procedural fog
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