#include "SortUtils.h"
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/SortKeyDirty.h"
#include "../ECS/Components/Relation.h"

namespace UIUtils::Sort
{
    void UpdateChildDepths(entt::registry* registry, entt::entity parent, u32& compoundDepth)
    {
        ZoneScoped;
        auto [parentRelation, parentSortKey] = registry->get<UIComponent::Relation, UIComponent::SortKey>(parent);

        for (const UI::UIChild& child : parentRelation.children)
        {
            UIComponent::SortKey& sortKey = registry->get<UIComponent::SortKey>(child.entId);
            sortKey.data.depthLayer = parentSortKey.data.depthLayer;
            sortKey.data.depth = parentSortKey.data.depth;
            sortKey.data.compoundDepth = compoundDepth++;

            UpdateChildDepths(registry, child.entId, compoundDepth);
        }
    }

    void MarkSortTreeDirty(entt::registry* registry, entt::entity entity)
    {
        auto relation = &registry->get<UIComponent::Relation>(entity);
        while (relation->parent != entt::null)
        {
            entity = relation->parent;
            relation = &registry->get<UIComponent::Relation>(entity);
        }

        if (!registry->has<UIComponent::SortKeyDirty>(entity))
            registry->emplace<UIComponent::SortKeyDirty>(entity);
    }
}