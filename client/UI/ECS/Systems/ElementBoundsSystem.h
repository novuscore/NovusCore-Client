#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class ElementBoundsSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}