#include "TransformUtils.h"
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"
#include "../ECS/Components/Dirty.h"

namespace UIUtils::Transform
{   
    void UpdateChildTransforms(entt::registry* registry, UIComponent::Transform* parent)
    {
        ZoneScoped;
        for (const UI::UIChild& child : parent->children)
        {
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);

            childTransform->position = UIUtils::Transform::GetAnchorPosition(parent, childTransform->anchor);
            if (childTransform->HasFlag(UI::TransformFlags::FILL_PARENTSIZE))
                childTransform->size = parent->size;

            UpdateChildTransforms(registry, childTransform);
        }
    }

    void MarkChildrenDirty(entt::registry* registry, const entt::entity entityId)
    {
        ZoneScoped;
        const auto transform = &registry->get<UIComponent::Transform>(entityId);
        for (const UI::UIChild& child : transform->children)
        {
            if (!registry->has<UIComponent::Dirty>(child.entId))
                registry->emplace<UIComponent::Dirty>(child.entId);

            MarkChildrenDirty(registry, child.entId);
        }
    }
}