#include "UpdateCullingSystem.h"
#include <entity/registry.hpp>

#include "../Components/Singletons/UIDataSingleton.h"
#include "../Components/Transform.h"
#include "../Components/Dirty.h"
#include "../Components/NotCulled.h"


namespace UISystem
{
    void UpdateCullingSystem::Update(entt::registry& registry)
    {
        const hvec2 UIRESOLUTION = registry.ctx<UISingleton::UIDataSingleton>().UIRESOLUTION;

        auto oldCulledView = registry.view<UIComponent::NotCulled, UIComponent::Dirty>();
        registry.remove<UIComponent::NotCulled>(oldCulledView.begin(), oldCulledView.end());

        auto cullView = registry.view<UIComponent::Transform, UIComponent::Dirty>();
        std::vector<entt::entity> notCulled;
        notCulled.reserve(cullView.size_hint());

        cullView.each([&](entt::entity entity, UIComponent::Transform& transform)
        {
            const hvec2 screenPosition = transform.anchorPosition + transform.position;
            const hvec2 offset = transform.localAnchor * transform.size;

            const hvec2 minBounds = screenPosition - offset;
            const hvec2 maxBounds = minBounds + transform.size;

            if (maxBounds.x < 0 || maxBounds.y < 0 || minBounds.x > UIRESOLUTION.x || minBounds.y > UIRESOLUTION.y)
                return;

            notCulled.push_back(entity);
        });
        registry.insert<UIComponent::NotCulled>(notCulled.begin(), notCulled.end());
    }
}