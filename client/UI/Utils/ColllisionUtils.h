#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
#include <entity/registry.hpp>
#include "../ECS/Components/BoundsDirty.h"

namespace UIUtils::Collision
{
    /*
    *   Recursively updates bounds of children and self.
    *   registry: Pointer to UI Registry.
    *   entityId: entity to start update from.
    *   updateParent: Whether or not to shallow update parents after updating our own bounds.
    *   NOT THREAD-SAFE.
    */
    void UpdateBounds(entt::registry* registry, entt::entity entityId, bool updateParent = true);
    /*
    *   Shallow update bounds by iterating over first-level children.
    *   registry: Pointer to UI Registry.
    *   entityId: entity to update bounds of.
    *   NOT THREAD-SAFE.
    */
    void ShallowUpdateBounds(entt::registry* registry, entt::entity entityId);

    inline static void MarkBoundsDirty(entt::registry* registry, entt::entity entityId)
    {
        if (!registry->has<UIComponent::BoundsDirty>(entityId))
            registry->emplace<UIComponent::BoundsDirty>(entityId);
    }
};