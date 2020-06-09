#pragma once
#include <NovusTypes.h>
#include "UITransform.h"

namespace UITransformUtils
{
    static const vec2 GetScreenPosition(const UITransform& transform)
    {
        return transform.position + transform.localPosition;
    };

    static const vec2 GetMinBounds(const UITransform& transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x - (transform.localAnchor.x * transform.size.x), screenPosition.y - (transform.localAnchor.y * transform.size.y));
    };

    static const vec2 GetMaxBounds(const UITransform& transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x + transform.size.x - (transform.localAnchor.x * transform.size.x), screenPosition.y + transform.size.y - (transform.localAnchor.y * transform.size.y));
    }

    static void AddChild(UITransform& transform, entt::entity childEntityId, UIElementData::UIElementType childElementType)
    {
        UITransform::UIChild newChild;
        newChild.entity = entt::to_integral(childEntityId);
        newChild.type = childElementType;

        transform.children.push_back(newChild);
    }

    static void RemoveChild(UITransform& transform, entt::entity childEntityId)
    {
        auto position = std::find_if(transform.children.begin(), transform.children.end(), [childEntityId](UITransform::UIChild& uiChild)
            {
                return uiChild.entity == entt::to_integral(childEntityId);
            });

        if (position != transform.children.end())
            transform.children.erase(position);
    }
};