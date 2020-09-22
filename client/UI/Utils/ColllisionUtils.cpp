#include "ColllisionUtils.h"
#include <tracy/Tracy.hpp>
#include "../ECS/Components/Collision.h"
#include "../ECS/Components/Transform.h"
#include "TransformUtils.h"

namespace UIUtils::Collision
{
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
}