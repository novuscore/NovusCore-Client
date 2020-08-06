#include "TransformUtils.h"
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"

namespace UIUtils::Transform
{
    void UpdateBounds(entt::registry* registry, UIComponent::Transform* transform)
    {
        ZoneScoped;

        transform->minBound = UIUtils::Transform::GetMinBounds(transform);
        transform->maxBound = UIUtils::Transform::GetMaxBounds(transform);

        if (transform->includeChildBounds)
        {

            for (const UI::UIChild& child : transform->children)
            {
                UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);

                if (childTransform->minBound.x < transform->minBound.x) { transform->minBound.x = childTransform->minBound.x; }
                if (childTransform->minBound.y < transform->minBound.y) { transform->minBound.y = childTransform->minBound.y; }

                if (childTransform->maxBound.x > transform->maxBound.x) { transform->maxBound.x = childTransform->maxBound.x; }
                if (childTransform->maxBound.y > transform->maxBound.y) { transform->maxBound.y = childTransform->maxBound.y; }
            }
        }

        if (transform->parent == entt::null)
            return;

        UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(transform->parent);
        if (parentTransform->includeChildBounds)
            UpdateBounds(registry, parentTransform);
    }

    void UpdateChildBounds(entt::registry* registry, UIComponent::Transform* transform)
    {
        ZoneScoped;
        transform->minBound = UIUtils::Transform::GetMinBounds(transform);
        transform->maxBound = UIUtils::Transform::GetMaxBounds(transform);

        if (transform->includeChildBounds)
        {
            for (const UI::UIChild& child : transform->children)
            {
                UpdateChildBounds(registry, &registry->get<UIComponent::Transform>(child.entId));
            }
        }

        if (transform->parent == entt::null)
            return;

        UpdateBounds(registry, &registry->get<UIComponent::Transform>(transform->parent));
    }
}