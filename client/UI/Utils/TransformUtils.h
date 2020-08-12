#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
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

    /*
    *   Recursively updates depths of children changing it by modifier.
    *   registry: Pointer to UI Registry.
    *   transform: Transform from which to start update.
    *   modifer: amount to modify depth by.
    */
    void UpdateChildDepths(entt::registry* registry, UIComponent::Transform* parent, u32 modifier);
    
    void UpdateChildTransforms(entt::registry* registry, UIComponent::Transform* parent);

    /*
    *   Recursively updates bounds of children and self.
    *   registry: Pointer to UI Registry.
    *   transform: Transform from which to start bounds update.
    *   updateParent: Whether or not to shallow update parents after updating our own bounds.
    */
    void UpdateBounds(entt::registry* registry, UIComponent::Transform* transform, bool updateParent = true);
    /*
    *   Shallow update bounds by non-recursively iterating over children. I.e we only go one level deep.
    *   registry: Pointer to UI Registry.
    *   transform: Transform to update bounds of.
    */
    void ShallowUpdateBounds(entt::registry* registry, UIComponent::Transform* transform);

    void MarkChildrenDirty(entt::registry* registry, const UIComponent::Transform* transform);

};