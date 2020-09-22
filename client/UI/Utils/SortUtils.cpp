#include "SortUtils.h"
#include <shared_mutex>
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"

namespace UIUtils::Sort
{
    void UpdateChildDepths(entt::registry* registry, entt::entity parent, i16 modifier)
    {
        ZoneScoped;
        auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();
        auto parentTransform = &registry->get<UIComponent::Transform>(parent);
        UI::DepthLayer depthLayer = registry->get<UIComponent::SortKey>(parent).data.depthLayer;
        for (const UI::UIChild& child : parentTransform->children)
        {
            std::lock_guard l(dataSingleton->GetMutex(child.entId));

            UIComponent::SortKey* sortKey = &registry->get<UIComponent::SortKey>(child.entId);
            sortKey->data.depth += modifier;
            sortKey->data.depthLayer = depthLayer;

            UpdateChildDepths(registry, child.entId, modifier);
        }
    }
}