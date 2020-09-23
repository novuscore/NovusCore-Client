#include "SortUtils.h"
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/Transform.h"

namespace UIUtils::Sort
{
    void UpdateChildDepths(entt::registry* registry, entt::entity parent, UI::DepthLayer depthLayer, i16 modifier)
    {
        ZoneScoped;
        auto parentTransform = &registry->get<UIComponent::Transform>(parent);
        for (const UI::UIChild& child : parentTransform->children)
        {
            UIComponent::SortKey* sortKey = &registry->get<UIComponent::SortKey>(child.entId);
            sortKey->data.depth += modifier;
            sortKey->data.depthLayer = depthLayer;

            UpdateChildDepths(registry, child.entId, depthLayer, modifier);
        }
    }
}