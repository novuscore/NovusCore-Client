#pragma once
#include <entity/fwd.hpp>

struct LocalplayerSingleton
{
    entt::entity entity = entt::null;
    
    bool autoRun = false;
};