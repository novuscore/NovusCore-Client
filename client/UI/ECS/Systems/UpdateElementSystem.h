#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    struct UIVertex
    {
        vec2 pos;
        vec2 uv;
    };

    class UpdateElementSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}