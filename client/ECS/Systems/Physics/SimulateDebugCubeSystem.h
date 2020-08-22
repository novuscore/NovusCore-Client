#pragma once
#include <entity/fwd.hpp>

class DebugRenderer;

constexpr f32 GRAVITY_SCALE = 10.0f;

class SimulateDebugCubeSystem
{
public:
    static void Init(entt::registry& registry);
    static void Update(entt::registry& registry, DebugRenderer* debugRenderer);
};