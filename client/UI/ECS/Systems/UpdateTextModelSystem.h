#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class UpdateTextModelSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}