#pragma once
#include <NovusTypes.h>
#include "../ECS/Components/Transform.h"

namespace UIUtils::Transform
{
    inline static const vec2 GetScreenPosition(const UIComponent::Transform& transform)
    {
        return transform.position + transform.localPosition;
    };

    inline static const vec2 GetMinBounds(const UIComponent::Transform& transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x - (transform.localAnchor.x * transform.size.x), screenPosition.y - (transform.localAnchor.y * transform.size.y));
    };

    inline static const vec2 GetMaxBounds(const UIComponent::Transform& transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x + transform.size.x - (transform.localAnchor.x * transform.size.x), screenPosition.y + transform.size.y - (transform.localAnchor.y * transform.size.y));
    }

    inline static const vec2 GetAnchorPosition(const UIComponent::Transform& transform, vec2 anchor)
    {
        return GetMinBounds(transform) + (transform.size * anchor);
    }

    inline static void AddChild(UIComponent::Transform& transform, entt::entity childEntityId, UI::UIElementType childElementType)
    {
        transform.children.push_back({ entt::to_integral(childEntityId) , childElementType });
    }

    inline static void RemoveChild(UIComponent::Transform& transform, entt::entity childEntityId)
    {
        auto itr = std::find_if(transform.children.begin(), transform.children.end(), [childEntityId](UI::UIChild& uiChild) { return uiChild.entity == entt::to_integral(childEntityId); });

        if (itr != transform.children.end())
            transform.children.erase(itr);
    }

    inline static void RemoveParent(UIComponent::Transform& transform)
    {
        transform.position = transform.position + transform.localPosition;
        transform.localPosition = vec2(0, 0);
        transform.parent = 0;
    }
};