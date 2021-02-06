#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
#include "../ECS/Components/Transform.h"

namespace UIUtils::Transform
{
    const hvec2 WindowPositionToUIPosition(const hvec2& WindowPosition);

    inline static const hvec2 GetScreenPosition(const UIComponent::Transform& transform)
    {
        return transform.anchorPosition + transform.position;
    };

    inline static const hvec2 GetMinBounds(const UIComponent::Transform& transform)
    {
        const hvec2 screenPosition = GetScreenPosition(transform);

        return screenPosition - (transform.localAnchor * transform.size);
    };

    inline static const hvec2 GetMaxBounds(const UIComponent::Transform& transform)
    {
        const hvec2 screenPosition = GetScreenPosition(transform);

        return screenPosition + transform.size - (transform.localAnchor * transform.size);
    }

    inline static const hvec2 GetInnerSize(const UIComponent::Transform* transform)
    {
        const hvec2 totalPadding = hvec2(transform->padding.left + transform->padding.right, transform->padding.top + transform->padding.bottom);

        return transform->size - totalPadding;
    }

    inline static const hvec2 GetAnchorPositionInElement(const UIComponent::Transform& transform, hvec2 anchor)
    {
        hvec2 minAnchorBound = GetMinBounds(transform) + hvec2(transform.padding.left, transform.padding.top);
        hvec2 adjustedSize = transform.size - hvec2(transform.padding.right, transform.padding.bottom);
        return minAnchorBound + adjustedSize * anchor;
    }

    hvec2 GetAnchorPositionOnScreen(hvec2 anchorPosition);

    void UpdateChildTransforms(entt::registry* registry, entt::entity entity);

    void UpdateChildPositions(entt::registry* registry, entt::entity entity);
};