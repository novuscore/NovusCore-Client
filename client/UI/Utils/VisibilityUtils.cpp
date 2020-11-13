#include "VisibilityUtils.h"
#include <shared_mutex>
#include <tracy/Tracy.hpp>
#include <entity/registry.hpp>
#include "../ECS/Components/Relation.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"

void UIUtils::Visibility::UpdateChildVisibility(entt::registry* registry, const entt::entity parent, bool parentVisibility)
{
    ZoneScoped;
    const UIComponent::Relation* relation = &registry->get<UIComponent::Relation>(parent);
    for (const UI::UIChild& child : relation->children)
    {
        UIComponent::Visibility* visibility = &registry->get<UIComponent::Visibility>(child.entId);
        if (!UpdateParentVisibility(visibility, parentVisibility))
            continue;

        const bool newVisibility = visibility->visibilityFlags == UI::VisibilityFlags::FULL_VISIBLE;
        UpdateChildVisibility(registry, child.entId, newVisibility);

        if (newVisibility)
            registry->emplace<UIComponent::Visible>(child.entId);
        else
            registry->remove<UIComponent::Visible>(child.entId);
    }
}
