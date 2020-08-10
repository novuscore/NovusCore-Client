#include "TransformUtils.h"
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"
#include "../ECS/Components/Dirty.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"

namespace UIUtils::Transform
{
    void UpdateChildDepths(entt::registry* registry, UIComponent::Transform* parent, u32 modifier)
    {
        ZoneScoped;
        auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();
        for (const UI::UIChild& child : parent->children)
        {
            std::lock_guard l(dataSingleton->GetMutex(child.entId));
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);
            childTransform->sortData.depth += modifier;

            UpdateChildDepths(registry, childTransform, modifier);
        }

    }
    
    void UpdateChildTransforms(entt::registry* registry, UIComponent::Transform* parent)
    {
        ZoneScoped;
        auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();
        for (const UI::UIChild& child : parent->children)
        {
            std::lock_guard l(dataSingleton->GetMutex(child.entId));
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);

            childTransform->position = UIUtils::Transform::GetAnchorPosition(parent, childTransform->anchor);
            if (childTransform->fillParentSize)
                childTransform->size = parent->size;

            if (!registry->has<UIComponent::Dirty>(child.entId))
                registry->emplace<UIComponent::Dirty>(child.entId);

            UpdateChildTransforms(registry, childTransform);
        }
    }

    void UpdateBounds(entt::registry* registry, UIComponent::Transform* transform, bool updateParent)
    {
        ZoneScoped;
        auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();
        transform->minBound = UIUtils::Transform::GetMinBounds(transform);
        transform->maxBound = UIUtils::Transform::GetMaxBounds(transform);

        for (const UI::UIChild& child : transform->children)
        {
            std::lock_guard l(dataSingleton->GetMutex(child.entId));
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

        std::lock_guard l(dataSingleton->GetMutex(transform->parent));
        UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(transform->parent);
        if (parentTransform->includeChildBounds)
            ShallowUpdateBounds(registry, parentTransform);
    }

    void ShallowUpdateBounds(entt::registry* registry, UIComponent::Transform* transform)
    {
        ZoneScoped;
        auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();
        transform->minBound = UIUtils::Transform::GetMinBounds(transform);
        transform->maxBound = UIUtils::Transform::GetMaxBounds(transform);

        if (transform->includeChildBounds)
        {
            for (const UI::UIChild& child : transform->children)
            {
                std::shared_lock l(dataSingleton->GetMutex(child.entId));
                UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);

                if (childTransform->minBound.x < transform->minBound.x) { transform->minBound.x = childTransform->minBound.x; }
                if (childTransform->minBound.y < transform->minBound.y) { transform->minBound.y = childTransform->minBound.y; }

                if (childTransform->maxBound.x > transform->maxBound.x) { transform->maxBound.x = childTransform->maxBound.x; }
                if (childTransform->maxBound.y > transform->maxBound.y) { transform->maxBound.y = childTransform->maxBound.y; }
            }
        }

        if (transform->parent == entt::null)
            return;

        std::lock_guard l(dataSingleton->GetMutex(transform->parent));
        UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(transform->parent);
        if (parentTransform->includeChildBounds)
            ShallowUpdateBounds(registry, parentTransform);
    }
}