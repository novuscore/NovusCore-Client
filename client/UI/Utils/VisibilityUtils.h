#pragma once
#include <NovusTypes.h>
#include "../ECS/Components/Visibility.h"
#include "../ECS/Components/Transform.h"
#include <entity/fwd.hpp>

namespace UIUtils::Visibility
{
    // Returns true if visibility changed.
    static inline bool UpdateVisibility(UIComponent::Visibility* visibility, bool visible)
    {
        if (visibility->visible == visible)
            return false;

        const bool visibilityChanged = visibility->parentVisible && visibility->visible != visibility->parentVisible && visible;
        visibility->visible = visible;

        return visibilityChanged;
    }

    // Returns true if visibility changed.
    static inline bool UpdateParentVisibility(UIComponent::Visibility* visibility, bool parentVisible)
    {
        if (visibility->parentVisible == parentVisible)
            return false;

        const bool visibilityChanged = visibility->parentVisible && visibility->visible != parentVisible && visibility->visible;
        visibility->parentVisible = parentVisible;

        return visibilityChanged;
    }

    static inline bool IsVisible(UIComponent::Visibility* visibility)
    {
        return visibility->parentVisible && visibility->visible;
    }

    void UpdateChildVisibility(entt::registry* registry, const UIComponent::Transform* parent, bool parentVisibility);
};