#include "ColllisionUtils.h"
#include <tracy/Tracy.hpp>
#include "../ECS/Components/Collision.h"
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Relation.h"
#include "TransformUtils.h"

namespace UIUtils::Collision
{
    void UpdateBounds(entt::registry* registry, entt::entity entityId, bool updateParent)
    {
        ZoneScoped;
        auto[collision, transform, relation] = registry->get<UIComponent::Collision, UIComponent::Transform, UIComponent::Relation>(entityId);
        collision.minBound = UIUtils::Transform::GetMinBounds(&transform);
        collision.maxBound = UIUtils::Transform::GetMaxBounds(&transform);

        for (const UI::UIChild& child : relation.children)
        {
            UpdateBounds(registry, child.entId, false);
            UIComponent::Collision* childCollision = &registry->get<UIComponent::Collision>(child.entId);

            if (!collision.HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS))
                continue;

            if (childCollision->minBound.x < collision.minBound.x) { collision.minBound.x = childCollision->minBound.x; }
            if (childCollision->minBound.y < collision.minBound.y) { collision.minBound.y = childCollision->minBound.y; }

            if (childCollision->maxBound.x > collision.maxBound.x) { collision.maxBound.x = childCollision->maxBound.x; }
            if (childCollision->maxBound.y > collision.maxBound.y) { collision.maxBound.y = childCollision->maxBound.y; }
        }

        if (!updateParent || relation.parent == entt::null)
            return;

        UIComponent::Collision* parentCollision = &registry->get<UIComponent::Collision>(relation.parent);
        if (parentCollision->HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS))
            ShallowUpdateBounds(registry, relation.parent);
    }

    void ShallowUpdateBounds(entt::registry* registry, entt::entity entityId)
    {
        ZoneScoped;
        auto [collision, transform, relation] = registry->get<UIComponent::Collision, UIComponent::Transform, UIComponent::Relation>(entityId);
        collision.minBound = UIUtils::Transform::GetMinBounds(&transform);
        collision.maxBound = UIUtils::Transform::GetMaxBounds(&transform);

        if (collision.HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS))
        {
            for (const UI::UIChild& child : relation.children)
            {
                UIComponent::Collision* childCollision = &registry->get<UIComponent::Collision>(child.entId);

                if (childCollision->minBound.x < collision.minBound.x) { collision.minBound.x = childCollision->minBound.x; }
                if (childCollision->minBound.y < collision.minBound.y) { collision.minBound.y = childCollision->minBound.y; }

                if (childCollision->maxBound.x > collision.maxBound.x) { collision.maxBound.x = childCollision->maxBound.x; }
                if (childCollision->maxBound.y > collision.maxBound.y) { collision.maxBound.y = childCollision->maxBound.y; }
            }
        }

        if (relation.parent == entt::null)
            return;

        UIComponent::Collision* parentCollision = &registry->get<UIComponent::Collision>(relation.parent);
        if (parentCollision->HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS))
            ShallowUpdateBounds(registry, relation.parent);
    }
}