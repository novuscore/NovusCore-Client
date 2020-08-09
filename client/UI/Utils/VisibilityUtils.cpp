#include "VisibilityUtils.h"
#include <tracy/Tracy.hpp>
#include <entity/registry.hpp>
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Visible.h"

void UIUtils::Visibility::UpdateChildVisibility(entt::registry* registry, const UIComponent::Transform* parent, bool parentVisibility)
{
    ZoneScoped;
    for (const UI::UIChild& child : parent->children)
    {
        UIComponent::Visibility* childVisibility = &registry->get<UIComponent::Visibility>(child.entId);

        if (!UpdateParentVisibility(childVisibility, parentVisibility))
            continue;

        const bool newVisibility = childVisibility->parentVisible && childVisibility->visible;
        const UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);
        UpdateChildVisibility(registry, childTransform, newVisibility);

        if (newVisibility)
            registry->emplace<UIComponent::Visible>(entt::entity(child.entId));
        else
            registry->remove<UIComponent::Visible>(entt::entity(child.entId));
    }
}
