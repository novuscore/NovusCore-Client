#include "SortUtils.h"
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/Relation.h"

namespace UIUtils::Sort
{
    void UpdateChildDepths(entt::registry* registry, entt::entity parent)
    {
        ZoneScoped;
        auto [parentRelation, parentSortKey] = registry->get<UIComponent::Relation, UIComponent::SortKey>(parent);

        printf("SortKey: %I64d\n", parentSortKey.key);

        u16 subDepth = parentSortKey.data.subDepth + 1;
        u16 childOrder = 0;
        for (const UI::UIChild& child : parentRelation.children)
        {
            UIComponent::SortKey& sortKey = registry->get<UIComponent::SortKey>(child.entId);
            sortKey.data.depthLayer = parentSortKey.data.depthLayer;
            sortKey.data.depth = parentSortKey.data.depth;
            sortKey.data.subDepth = subDepth;
            sortKey.data.childOrder = childOrder++;


            UpdateChildDepths(registry, child.entId);
        }
    }
}