#include "VisibilityUtils.h"
#include <shared_mutex>
#include <tracy/Tracy.hpp>
#include <entity/registry.hpp>
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"

void UIUtils::Visibility::UpdateChildVisibility(entt::registry* registry, const entt::entity parent, bool parentVisibility)
{
    ZoneScoped;
    auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();
    const UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(parent);
    for (const UI::UIChild& child : parentTransform->children)
    {
        std::lock_guard l(dataSingleton->GetMutex(child.entId));

        UIComponent::Visibility* childVisibility = &registry->get<UIComponent::Visibility>(child.entId);
        if (!UpdateParentVisibility(childVisibility, parentVisibility))
            continue;

        const bool newVisibility = childVisibility->parentVisible && childVisibility->visible;
        UpdateChildVisibility(registry, child.entId, newVisibility);

        dataSingleton->visibilityToggleQueue.enqueue(child.entId);
    }
}
