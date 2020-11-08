#pragma once
#include <NovusTypes.h>

class asIScriptFunction;

namespace UI
{
    enum TransformEventsFlags : u8
    {
        UIEVENTS_FLAG_NONE = 1 << 0,

        UIEVENTS_FLAG_CLICKABLE = 1 << 1,
        UIEVENTS_FLAG_DRAGGABLE = 1 << 2,
        UIEVENTS_FLAG_FOCUSABLE = 1 << 3,
        UIEVENTS_FLAG_RESIZEABLE = 1 << 4,

        UIEVENTS_FLAG_DRAGLOCK_X = 1 << 5,
        UIEVENTS_FLAG_DRAGLOCK_Y = 1 << 6
    };
}

namespace UIComponent
{
    // We need to define structs for event data, so we can pass data into callbacks for angelscript
    struct TransformEvents
    {
    public:
        TransformEvents() { }

        u8 flags = 0;
        asIScriptFunction* onClickCallback = nullptr;

        asIScriptFunction* onDragStartedCallback = nullptr;
        asIScriptFunction* onDragEndedCallback = nullptr;
        
        asIScriptFunction* onFocusGainedCallback = nullptr;
        asIScriptFunction* onFocusLostCallback = nullptr;
        
        asIScriptFunction* onHoverStartedCallback = nullptr;
        asIScriptFunction* onHoverEndedCallback = nullptr;

        inline void SetFlag(const UI::TransformEventsFlags inFlags) { flags |= inFlags; }
        inline void UnsetFlag(const UI::TransformEventsFlags inFlags) { flags &= ~inFlags; }
        inline bool HasFlag(const UI::TransformEventsFlags inFlags) const { return (flags & inFlags) == inFlags; }
        inline const bool IsClickable() const { return (flags & UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE) == UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE; }
        inline const bool IsDraggable() const { return (flags & UI::TransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE) == UI::TransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE; }
        inline const bool IsFocusable() const { return (flags & UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE) == UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE; }
    };
}