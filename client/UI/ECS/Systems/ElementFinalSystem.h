#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class ElementFinalSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}