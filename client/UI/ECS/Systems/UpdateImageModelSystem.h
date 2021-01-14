#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class UpdateImageModelSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}