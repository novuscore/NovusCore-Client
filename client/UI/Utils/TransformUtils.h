#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
#include "../ECS/Components/Transform.h"

namespace UIUtils::Transform
{
    const hvec2 WindowPositionToUIPosition(const hvec2& WindowPosition);

    inline static const hvec2 GetScreenPosition(const UIComponent::Transform* transform)
    {
        return transform->anchorPosition + transform->position;
    };

    inline static const hvec2 GetMinBounds(const UIComponent::Transform* transform)
    {
        const hvec2 screenPosition = GetScreenPosition(transform);

        return hvec2(screenPosition.x - (transform->localAnchor.x * transform->size.x), screenPosition.y - (transform->localAnchor.y * transform->size.y));
    };

    inline static const hvec2 GetMaxBounds(const UIComponent::Transform* transform)
    {
        const hvec2 screenPosition = GetScreenPosition(transform);

        return hvec2(screenPosition.x + transform->size.x - (transform->localAnchor.x * transform->size.x), screenPosition.y + transform->size.y - (transform->localAnchor.y * transform->size.y));
    }

    inline static const hvec2 GetAnchorPositionInElement(const UIComponent::Transform* transform, hvec2 anchor)
    {
        return GetMinBounds(transform) + (transform->size * anchor);
    }

    hvec2 GetAnchorPositionOnScreen(hvec2 anchorPosition);

    void UpdateChildTransforms(entt::registry* registry, entt::entity entity);
};