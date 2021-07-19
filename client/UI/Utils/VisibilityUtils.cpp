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
    for (const entt::entity childId : relation->children)
    {
        UIComponent::Visibility* visibility = &registry->get<UIComponent::Visibility>(childId);
        if (!UpdateParentVisibility(visibility, parentVisibility))
            continue;

        const bool newVisibility = visibility->visibilityFlags == UI::VisibilityFlags::FULL_VISIBLE;
        UpdateChildVisibility(registry, childId, newVisibility);

        if (newVisibility)
            registry->emplace<UIComponent::Visible>(childId);
        else
            registry->remove<UIComponent::Visible>(childId);
    }
}
