#pragma once
#include <NovusTypes.h>
#include "../ECS/Components/Transform.h"

namespace UIUtils::Transform
{
    inline static const vec2 GetScreenPosition(const UIComponent::Transform* transform)
    {
        return transform->position + transform->localPosition;
    };

    inline static const vec2 GetMinBounds(const UIComponent::Transform* transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x - (transform->localAnchor.x * transform->size.x), screenPosition.y - (transform->localAnchor.y * transform->size.y));
    };

    inline static const vec2 GetMaxBounds(const UIComponent::Transform* transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x + transform->size.x - (transform->localAnchor.x * transform->size.x), screenPosition.y + transform->size.y - (transform->localAnchor.y * transform->size.y));
    }

    inline static const vec2 GetAnchorPosition(const UIComponent::Transform* transform, vec2 anchor)
    {
        return GetMinBounds(transform) + (transform->size * anchor);
    }

    inline static void AddChild(UIComponent::Transform* transform, UIComponent::Transform* child)
    {
        transform->children.push_back({ child->sortData.entId , child->sortData.type });
    }

    inline static void RemoveChild(UIComponent::Transform* transform, UIComponent::Transform* child)
    {
        if (child->parent != transform->sortData.entId)
            return;

        auto itr = std::find_if(transform->children.begin(), transform->children.end(), [child](UI::UIChild& uiChild) { return uiChild.entId == child->sortData.entId; });
        if (itr != transform->children.end())
            transform->children.erase(itr);

        child->position = child->position + child->localPosition;
        child->localPosition = vec2(0, 0);
        child->parent = entt::null;
    }
};