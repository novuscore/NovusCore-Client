#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class UpdateBoundsSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}