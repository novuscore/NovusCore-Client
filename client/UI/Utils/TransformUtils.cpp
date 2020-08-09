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
            if (childTransform->fillParentSize)
                childTransform->size = parent->size;

            if (!registry->has<UIComponent::Dirty>(child.entId))
                registry->emplace<UIComponent::Dirty>(child.entId);

            UpdateChildTransforms(registry, childTransform);
        }

        UpdateBounds(registry, parent);
    }

    void UpdateBounds(entt::registry* registry, UIComponent::Transform* transform, bool updateParent)
    {
        ZoneScoped;
        transform->minBound = UIUtils::Transform::GetMinBounds(transform);
        transform->maxBound = UIUtils::Transform::GetMaxBounds(transform);

        for (const UI::UIChild& child : transform->children)
        {
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);
            UpdateBounds(registry, childTransform, false);

            if (!transform->includeChildBounds)
                continue;

            if (childTransform->minBound.x < transform->minBound.x) { transform->minBound.x = childTransform->minBound.x; }
            if (childTransform->minBound.y < transform->minBound.y) { transform->minBound.y = childTransform->minBound.y; }

            if (childTransform->maxBound.x > transform->maxBound.x) { transform->maxBound.x = childTransform->maxBound.x; }
            if (childTransform->maxBound.y > transform->maxBound.y) { transform->maxBound.y = childTransform->maxBound.y; }
        }

        if (!updateParent || transform->parent == entt::null)
            return;

        UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(transform->parent);
        if (parentTransform->includeChildBounds)
            ShallowUpdateBounds(registry, parentTransform);
    }

    void ShallowUpdateBounds(entt::registry* registry, UIComponent::Transform* transform)
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
            ShallowUpdateBounds(registry, parentTransform);
    }
}