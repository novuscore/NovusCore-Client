#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class ElementCullingSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}