#pragma once
#include <NovusTypes.h>
#include "BaseElement.h"

namespace UIScripting
{
    class EventElement : public BaseElement
    {
    public:
        EventElement(UI::ElementType elementType, const std::string& name, bool collisionEnabled = true, u8 eventFlags = 0);

        static void RegisterType();

        template<class T>
        static void RegisterBase()
        {
            // TransformEvents Functions
            i32 r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(T, IsClickable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetClickable(bool clickable)", asMETHOD(T, SetClickable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsDraggable()", asMETHOD(T, IsDraggable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetDraggable(bool draggable)", asMETHOD(T, SetDraggable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(T, IsFocusable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetFocusable(bool focusable)", asMETHOD(T, SetFocusable)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("bool IsEnabled()", asMETHOD(T, IsEnabled)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetEnabled(bool enabled)", asMETHOD(T, SetEnabled)); assert(r >= 0);

            r = RegisterEvents("EventElement");
        }

    private:
        static i32 RegisterEvents(std::string typeName)
        {
            i32 r = ScriptEngine::RegisterScriptClassFunction("void OnClick(" + typeName + "EventCallback@ cb)", asMETHOD(EventElement, SetOnClickCallback)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void OnDragStarted(" + typeName + "EventCallback@ cb)", asMETHOD(EventElement, SetOnDragStartedCallback)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void OnDragEnded(" + typeName + "EventCallback@ cb)", asMETHOD(EventElement, SetOnDragEndedCallback)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void OnFocusGained(" + typeName + "EventCallback@ cb)", asMETHOD(EventElement, SetOnFocusGainedCallback)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void OnFocusLost(" + typeName + "EventCallback@ cb)", asMETHOD(EventElement, SetOnFocusLostCallback)); assert(r >= 0);

            return r;
        }
    protected:
        template<class T>
        static i32 RegisterEventBase(std::string typeName)
        {
            i32 r = ScriptEngine::RegisterScriptInheritance<BaseElement, T>("BaseElement"); assert(r >= 0);
            r = ScriptEngine::RegisterScriptInheritance<EventElement, T>("EventElement"); assert(r >= 0);
            r = ScriptEngine::RegisterScriptFunctionDef("void " + typeName + "EventCallback(" + typeName + "@ element)"); assert(r >= 0);
            r = RegisterEvents(typeName); assert(r >= 0);
            
            return r;
        }

    public:
        // TransformEvents Functions
        const bool IsClickable() const;
        void SetClickable(bool clickable);
        const bool IsDraggable() const;
        void SetDraggable(bool draggable);
        const bool IsFocusable() const;
        void SetFocusable(bool focusable);

        const bool IsEnabled() const;
        void SetEnabled(bool enabled);

        void SetOnClickCallback(asIScriptFunction* callback);

        void SetOnDragStartedCallback(asIScriptFunction* callback);
        void SetOnDragEndedCallback(asIScriptFunction* callback);

        void SetOnFocusGainedCallback(asIScriptFunction* callback);
        void SetOnFocusLostCallback(asIScriptFunction* callback);

        // Input Functions
        virtual void OnClick(vec2 mousePosition) {}
        virtual void OnDrag() {}
        virtual bool OnKeyInput(i32 key) { return false; }
        virtual bool OnCharInput(char c) { return false; }
    };
}
