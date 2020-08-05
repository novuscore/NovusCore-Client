#pragma once
#include <NovusTypes.h>
#include <angelscript.h>
#include "../../../Scripting/ScriptEngine.h"

namespace UI
{
    enum UITransformEventsFlags
    {
        UIEVENTS_FLAG_NONE = 1 << 0,
        UIEVENTS_FLAG_CLICKABLE = 1 << 1,
        UIEVENTS_FLAG_DRAGGABLE = 1 << 2,
        UIEVENTS_FLAG_FOCUSABLE = 1 << 3
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
        asIScriptFunction* onDraggedCallback = nullptr;
        asIScriptFunction* onFocusedCallback = nullptr;
        asIScriptFunction* onUnfocusedCallback = nullptr;
        void* asObject = nullptr;

        // Usually Components do not store logic, however this is an exception
    private:
        void _OnEvent(asIScriptFunction* callback)
        {
            asIScriptContext* context = ScriptEngine::GetScriptContext();
            {
                context->Prepare(callback);
                {
                    context->SetArgObject(0, asObject);
                }
                context->Execute();
            }
        }
    public:
        void OnClick()
        {
            if (!onClickCallback)
                return;

            _OnEvent(onClickCallback);
        }
        void OnDragged()
        {
            if (!onDraggedCallback)
                return;

            _OnEvent(onDraggedCallback);
        }
        void OnFocused()
        {
            if (!onFocusedCallback)
                return;

            _OnEvent(onFocusedCallback);
        }
        void OnUnfocused()
        {
            if (!onUnfocusedCallback)
                return;

            _OnEvent(onUnfocusedCallback);
        }

        void SetFlag(const UI::UITransformEventsFlags inFlags) { flags |= inFlags; }
        void UnsetFlag(const UI::UITransformEventsFlags inFlags) { flags &= ~inFlags; }
        const bool IsClickable() const { return (flags & UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE) == UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE; }
        const bool IsDraggable() const { return (flags & UI::UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE) == UI::UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE; }
        const bool IsFocusable() const { return (flags & UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE) == UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE; }
    };
}