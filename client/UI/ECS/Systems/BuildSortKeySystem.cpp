#include "BuildSortKeySystem.h"
#include <entity/registry.hpp>

#include "../../../Utils/ServiceLocator.h"
#include "../Components/Relation.h"
#include "../Components/SortKey.h"
#include "../Components/SortKeyDirty.h"

#include "../../Utils/SortUtils.h"

namespace UISystem
{
    void BuildSortKeySystem::Update(entt::registry& registry)
    {
        auto sortView = registry.view<UIComponent::Relation, UIComponent::SortKey, UIComponent::SortKeyDirty>();
        sortView.each([&](entt::entity entity, UIComponent::Relation& relation, UIComponent::SortKey& sortKey)
        {
            sortKey.data.compoundDepth = 0;

            u32 compoundDepth = 1;
            UIUtils::Sort::UpdateChildDepths(&registry, entity, compoundDepth);
        });

    }
}