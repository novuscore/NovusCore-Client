#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    struct UIVertex
    {
        vec2 pos;
        vec2 uv;
    };

    class UpdateRenderingSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}