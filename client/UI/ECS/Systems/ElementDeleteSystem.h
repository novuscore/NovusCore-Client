#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class ElementDeleteSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}