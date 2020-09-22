#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Dirty.h"
#include "../ECS/Components/BoundsDirty.h"

namespace UIUtils::Transform
{
    inline static const hvec2 GetScreenPosition(const UIComponent::Transform* transform)
    {
        return transform->position + transform->localPosition;
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

    inline static const vec2 GetAnchorPosition(const UIComponent::Transform* transform, hvec2 anchor)
    {
        return GetMinBounds(transform) + (transform->size * anchor);
    }

    inline static void RemoveChild(entt::registry* registry, entt::entity parent, entt::entity child)
    {
        UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child);
        if (childTransform->parent != parent)
            return;
        UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(parent);

        auto itr = std::find_if(parentTransform->children.begin(), parentTransform->children.end(), [child](UI::UIChild& uiChild) { return uiChild.entId == child; });
        if (itr != parentTransform->children.end())
            parentTransform->children.erase(itr);

        childTransform->position = childTransform->position + childTransform->localPosition;
        childTransform->localPosition = vec2(0, 0);
        childTransform->parent = entt::null;
    }

    void UpdateChildTransforms(entt::registry* registry, UIComponent::Transform* parent);

    void MarkChildrenDirty(entt::registry* registry, const UIComponent::Transform* transform);

    inline static void MarkDirty(entt::registry* registry, entt::entity entId)
    {
        if (!registry->has<UIComponent::Dirty>(entId))
            registry->emplace<UIComponent::Dirty>(entId);
    }

};