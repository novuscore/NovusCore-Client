#pragma once
#include <NovusTypes.h>
#include "../ECS/Components/Visibility.h"
#include <entity/fwd.hpp>

namespace UIUtils::Visibility
{
    // Returns true if visibility changed.
    static inline bool UpdateVisibility(UIComponent::Visibility* visibility, bool visible)
    {
        if (visibility->HasFlag(UI::VisibilityFlags::VISIBLE) == visible)
            return false;
        
        const bool originalVisible = visibility->visibilityFlags == UI::VisibilityFlags::FULL_VISIBLE;
        visibility->ToggleFlag(UI::VisibilityFlags::VISIBLE);
        const bool newVisible = visibility->visibilityFlags == UI::VisibilityFlags::FULL_VISIBLE;

        return originalVisible != newVisible;
    }

    // Returns true if visibility changed.
    static inline bool UpdateParentVisibility(UIComponent::Visibility* visibility, bool parentVisible)
    {
        if (visibility->HasFlag(UI::VisibilityFlags::PARENTVISIBLE) == parentVisible)
            return false;

        const bool originalVisible = visibility->visibilityFlags == UI::VisibilityFlags::FULL_VISIBLE;
        visibility->ToggleFlag(UI::VisibilityFlags::PARENTVISIBLE);
        const bool newVisible = visibility->visibilityFlags == UI::VisibilityFlags::FULL_VISIBLE;

        return originalVisible != newVisible;
    }

    static inline bool IsVisible(const UIComponent::Visibility* visibility)
    {
        return visibility->visibilityFlags == UI::VisibilityFlags::FULL_VISIBLE;
    }

    /*
    *   THREAD-SAFE.
    */
    void UpdateChildVisibility(entt::registry* registry, const entt::entity parent, bool parentVisibility);
};