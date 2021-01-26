#pragma once
#include <NovusTypes.h>

class asIScriptFunction;

namespace UI
{
    enum TransformEventsFlags : u8
    {
        FLAG_NONE = 1 << 0,

        FLAG_CLICKABLE = 1 << 1,
        FLAG_DRAGGABLE = 1 << 2,
        FLAG_FOCUSABLE = 1 << 3,
        FLAG_RESIZEABLE = 1 << 4,

        FLAG_DRAGLOCK_X = 1 << 5,
        FLAG_DRAGLOCK_Y = 1 << 6
    };

    enum TransformEventState : u8
    {
        STATE_NORMAL = 0,
        STATE_FOCUSED = 1 << 0,
        STATE_HOVERED = 1 << 1,
        STATE_PRESSED = 1 << 2
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
        u8 state = 0;

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

        inline void SetState(const UI::TransformEventState inState) { state |= inState; }
        inline void UnsetState(const UI::TransformEventState inState) { state &= ~inState; }
        inline bool HasState(const UI::TransformEventState inState) const { return (state & inState) == inState; }
    };
}