#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class UpdateElementSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}