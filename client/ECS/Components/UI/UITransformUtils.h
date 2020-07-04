#pragma once
#include <NovusTypes.h>
#include "UITransform.h"

namespace UITransformUtils
{
    inline static const vec2 GetScreenPosition(const UITransform& transform)
    {
        return transform.position + transform.localPosition;
    };

    inline static const vec2 GetMinBounds(const UITransform& transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x - (transform.localAnchor.x * transform.size.x), screenPosition.y - (transform.localAnchor.y * transform.size.y));
    };

    inline static const vec2 GetMaxBounds(const UITransform& transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x + transform.size.x - (transform.localAnchor.x * transform.size.x), screenPosition.y + transform.size.y - (transform.localAnchor.y * transform.size.y));
    }

    inline static const vec2 GetAnchorPosition(const UITransform& transform, vec2 anchor)
    {
        return GetMinBounds(transform) + (transform.size * anchor);
    }

    inline static void AddChild(UITransform& transform, entt::entity childEntityId, UI::UIElementType childElementType)
    {
        transform.children.push_back({ entt::to_integral(childEntityId) , childElementType });
    }

    inline static void RemoveChild(UITransform& transform, entt::entity childEntityId)
    {
        auto position = std::find_if(transform.children.begin(), transform.children.end(), [childEntityId](UIChild& uiChild)
            {
                return uiChild.entity == entt::to_integral(childEntityId);
            });

        if (position != transform.children.end())
            transform.children.erase(position);
    }
};