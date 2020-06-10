#pragma once
#include <entity/fwd.hpp>

class ClientRenderer;
class RenderModelSystem
{
public:
    static void Update(entt::registry& registry, ClientRenderer* clientRenderer);
};