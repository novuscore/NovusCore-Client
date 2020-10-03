#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>

namespace UIUtils
{
    void ClearAllElements();

    void MarkChildrenForDestruction(entt::registry* registry, entt::entity entityId);
};