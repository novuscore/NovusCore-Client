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
    const UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(parent);
    for (const UI::UIChild& child : parentTransform->children)
    {
        UIComponent::Visibility* childVisibility = &registry->get<UIComponent::Visibility>(child.entId);
        if (!UpdateParentVisibility(childVisibility, parentVisibility))
            continue;

        const bool newVisibility = childVisibility->visibilityFlags == UI::VisibilityFlags::FULL_VISIBLE;
        UpdateChildVisibility(registry, child.entId, newVisibility);

        if (newVisibility)
            registry->emplace<UIComponent::Visible>(child.entId);
        else
            registry->remove<UIComponent::Visible>(child.entId);
    }
}
