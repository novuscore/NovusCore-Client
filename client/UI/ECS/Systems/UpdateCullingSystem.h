#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class UpdateCullingSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}