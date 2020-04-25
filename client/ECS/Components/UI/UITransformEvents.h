#pragma once
#include <NovusTypes.h>
#include <angelscript.h>
#include "../../../Scripting/ScriptEngine.h"

enum UITransformEventsFlags
{
    UIEVENTS_FLAG_NONE = 1 << 0,
    UIEVENTS_FLAG_CLICKABLE = 1 << 1,
    UIEVENTS_FLAG_DRAGGABLE = 1 << 2,
    UIEVENTS_FLAG_FOCUSABLE = 1 << 3

};

// We need to define structs for event data, so we can pass data into callbacks for angelscript
struct UITransformEvents
{
public:
    UITransformEvents() : flags(), onClickCallback(nullptr), onDraggedCallback(nullptr), onFocusedCallback(nullptr) { }

    u8 flags;
    asIScriptFunction* onClickCallback;
    asIScriptFunction* onDraggedCallback;
    asIScriptFunction* onFocusedCallback;

    // Usually Components do not store logic, however this is an exception
private:
    void _OnEvent(void* thisPtr, asIScriptFunction* callback)
    {
        asIScriptContext* context = ScriptEngine::GetScriptContext();
        {
            context->Prepare(callback);
            {
                context->SetArgObject(0, this);
            }
            context->Execute();
        }
    }
public:
    void OnClick(void* thisPtr)
    {
        if (!onClickCallback)
            return;
        
        _OnEvent(thisPtr, onClickCallback);
    }
    void OnDragged(void* thisPtr)
    {
        if (!onDraggedCallback)
            return;
        
        _OnEvent(thisPtr, onDraggedCallback);
    }    
    void OnFocused(void* thisPtr)
    {
        if (!onFocusedCallback)
            return;
        
        _OnEvent(thisPtr, onFocusedCallback);
    }

    void SetFlag(const UITransformEventsFlags inFlags) { flags |= inFlags; }
    void UnsetFlag(const UITransformEventsFlags inFlags) { flags &= ~inFlags; }
    const bool IsClickable() const { return (flags & UIEVENTS_FLAG_CLICKABLE) == UIEVENTS_FLAG_CLICKABLE; }
    const bool IsDraggable() const { return (flags & UIEVENTS_FLAG_DRAGGABLE) == UIEVENTS_FLAG_DRAGGABLE; }
    const bool IsFocusable() const { return (flags & UIEVENTS_FLAG_FOCUSABLE) == UIEVENTS_FLAG_FOCUSABLE; }
};