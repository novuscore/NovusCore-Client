#include "SortUtils.h"
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/Relation.h"

namespace UIUtils::Sort
{
    void UpdateChildDepths(entt::registry* registry, entt::entity parent, UI::DepthLayer depthLayer, i16 modifier)
    {
        ZoneScoped;
        auto relation = &registry->get<UIComponent::Relation>(parent);
        for (const UI::UIChild& child : relation->children)
        {
            UIComponent::SortKey* sortKey = &registry->get<UIComponent::SortKey>(child.entId);
            sortKey->data.depth += modifier;
            sortKey->data.depthLayer = depthLayer;

            UpdateChildDepths(registry, child.entId, depthLayer, modifier);
        }
    }
}