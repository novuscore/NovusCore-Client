#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class FinalCleanUpSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}