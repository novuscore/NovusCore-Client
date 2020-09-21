#include "TransformUtils.h"
#include <tracy/Tracy.hpp>
#include <shared_mutex>
#include "entity/registry.hpp"
#include "../ECS/Components/Dirty.h"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/Collision.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"

namespace UIUtils::Transform
{
    void UpdateChildDepths(entt::registry* registry, entt::entity parent, i16 modifier)
    {
        ZoneScoped;
        auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();
        auto parentTransform = &registry->get<UIComponent::Transform>(parent);
        UI::DepthLayer depthLayer = registry->get<UIComponent::SortKey>(parent).data.depthLayer;
        for (const UI::UIChild& child : parentTransform->children)
        {
            std::lock_guard l(dataSingleton->GetMutex(child.entId));

            UIComponent::SortKey* sortKey = &registry->get<UIComponent::SortKey>(child.entId);
            sortKey->data.depth += modifier;
            sortKey->data.depthLayer = depthLayer;

            UpdateChildDepths(registry, child.entId, modifier);
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
            if (childTransform->HasFlag(UI::TransformFlags::FILL_PARENTSIZE))
                childTransform->size = parent->size;

            UpdateChildTransforms(registry, childTransform);
        }
    }

    void UpdateBounds(entt::registry* registry, entt::entity entityId, bool updateParent)
    {
        ZoneScoped;
        UIComponent::Collision* collision = &registry->get<UIComponent::Collision>(entityId);
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(entityId);
        collision->minBound = UIUtils::Transform::GetMinBounds(transform);
        collision->maxBound = UIUtils::Transform::GetMaxBounds(transform);

        for (const UI::UIChild& child : transform->children)
        {
            UpdateBounds(registry, child.entId, false);
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);
            UIComponent::Collision* childCollision = &registry->get<UIComponent::Collision>(child.entId);

            if (!childCollision->HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS))
                continue;

            if (childCollision->minBound.x < collision->minBound.x) { collision->minBound.x = childCollision->minBound.x; }
            if (childCollision->minBound.y < collision->minBound.y) { collision->minBound.y = childCollision->minBound.y; }

            if (childCollision->maxBound.x > collision->maxBound.x) { collision->maxBound.x = childCollision->maxBound.x; }
            if (childCollision->maxBound.y > collision->maxBound.y) { collision->maxBound.y = childCollision->maxBound.y; }
        }

        if (!updateParent || transform->parent == entt::null)
            return;

        UIComponent::Collision* parentCollision = &registry->get<UIComponent::Collision>(transform->parent);
        if (parentCollision->HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS))
            ShallowUpdateBounds(registry, transform->parent);
    }

    void ShallowUpdateBounds(entt::registry* registry, entt::entity entityId)
    {
        ZoneScoped;
        auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();
        UIComponent::Collision* collision = &registry->get<UIComponent::Collision>(entityId);
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(entityId);
        collision->minBound = UIUtils::Transform::GetMinBounds(transform);
        collision->maxBound = UIUtils::Transform::GetMaxBounds(transform);

        if (collision->HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS))
        {
            for (const UI::UIChild& child : transform->children)
            {
                UIComponent::Collision* childCollision = &registry->get<UIComponent::Collision>(child.entId);

                if (childCollision->minBound.x < collision->minBound.x) { collision->minBound.x = childCollision->minBound.x; }
                if (childCollision->minBound.y < collision->minBound.y) { collision->minBound.y = childCollision->minBound.y; }

                if (childCollision->maxBound.x > collision->maxBound.x) { collision->maxBound.x = childCollision->maxBound.x; }
                if (childCollision->maxBound.y > collision->maxBound.y) { collision->maxBound.y = childCollision->maxBound.y; }
            }
        }

        if (transform->parent == entt::null)
            return;

        UIComponent::Collision* parentCollision = &registry->get<UIComponent::Collision>(transform->parent);
        if (parentCollision->HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS))
            ShallowUpdateBounds(registry, transform->parent);
    }

    void MarkChildrenDirty(entt::registry* registry, const UIComponent::Transform* transform)
    {
        auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();

        for (const UI::UIChild& child : transform->children)
        {
            const auto childTransform = &registry->get<UIComponent::Transform>(child.entId);
            MarkChildrenDirty(registry, childTransform);
            
            dataSingleton->dirtyQueue.enqueue(child.entId);
        }
    }
}