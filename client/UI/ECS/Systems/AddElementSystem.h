#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class AddElementSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}