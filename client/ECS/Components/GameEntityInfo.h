#pragma once
#include <NovusTypes.h>

enum class GameEntityType : u8
{
    NONE,
    GAMEOBJECT,
    CREATURE,
    PLAYER,
    ITEM
};

struct GameEntityInfo
{
    GameEntityType type;
    u32 entryId;
};