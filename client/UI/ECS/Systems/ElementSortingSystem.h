#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class ElementSortingSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}