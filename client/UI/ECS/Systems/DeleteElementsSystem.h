#pragma once
#include <entity/fwd.hpp>

namespace UISystem
{
    class DeleteElementsSystem
    {
    public:
        static void Update(entt::registry& registry);
    };
}