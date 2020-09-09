#pragma once
#include <entity/fwd.hpp>

class MovementSystem
{
public:
    static void Init(entt::registry& registry);
    static void Update(entt::registry& registry);
};