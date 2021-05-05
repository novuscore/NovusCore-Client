#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
#include "../ECS/Components/Dirty.h"

namespace UIUtils
{
    void ClearAllElements();

    void MarkChildrenDirty(entt::registry* registry, const entt::entity entityId);
    void MarkChildrenForDestruction(entt::registry* registry, entt::entity entityId);
    
    void AddChild(entt::registry* registry, entt::entity parent, entt::entity child);
    void RemoveFromParent(entt::registry* registry, entt::entity child);

    inline static void MarkDirty(entt::registry* registry, entt::entity entId)
    {
        if (!registry->has<UIComponent::Dirty>(entId))
            registry->emplace<UIComponent::Dirty>(entId);
    }
};