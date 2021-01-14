#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class LoadTexturesSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}