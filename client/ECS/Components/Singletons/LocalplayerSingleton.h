#pragma once
#include <entity/fwd.hpp>

struct LocalplayerSingleton
{
    entt::entity entity = entt::null;
    
    struct MovementProperties
    {
        u32 autoRun : 1;
        u32 canJump : 1;
        u32 canChangeDirection : 1;

    } movementProperties;
};