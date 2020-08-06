#pragma once
#include <NovusTypes.h>
#include "../ECS/Components/Visibility.h"

namespace UIUtils::Visibility
{
    // Returns true if visibility changed.
    static inline bool UpdateVisibility(UIComponent::Visibility* visibility, bool visible)
    {
        if (visibility->visible == visible)
            return false;

        const bool oldVisibility = visibility->parentVisible && visibility->visible;
        const bool newVisibility = visibility->parentVisible && visible;
        visibility->visible = visible;

        return oldVisibility != visible;
    }

    // Returns true if visibility changed.
    static inline bool UpdateParentVisibility(UIComponent::Visibility* visibility, bool parentVisible)
    {
        if (visibility->parentVisible == parentVisible)
            return false;

        const bool oldVisibility = visibility->parentVisible && visibility->visible;
        const bool newVisibility = parentVisible && visibility->visible;
        visibility->parentVisible = parentVisible;

        return oldVisibility != newVisibility;
    }

    static inline bool IsVisible(UIComponent::Visibility* visibility)
    {
        return visibility->parentVisible && visibility->visible;
    }
};