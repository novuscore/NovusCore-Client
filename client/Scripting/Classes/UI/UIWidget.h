#pragma once
#include <NovusTypes.h>
#include "../../ScriptEngine.h"
#include "../../../UI/Widget/Widget.h"

class UIWidget
{
public:
    UIWidget(UI::Widget* widget)
    {
        _widget = widget;
    }
    static void RegisterType();

    template <class T>
    static void RegisterBase()
    {
        i32 r = ScriptEngine::RegisterScriptClassFunction("string GetTypeName()", asMETHOD(T, GetTypeName)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetPosition(vec2 pos, float depth = 0)", asMETHOD(T, SetPosition)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetSize(vec2 size)", asMETHOD(T, SetSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetPosition()", asMETHOD(T, GetPosition)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetSize()", asMETHOD(T, GetSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetDepth()", asMETHOD(T, GetDepth)); assert(r >= 0);
    }

    virtual std::string GetTypeName();
    virtual void SetPosition(const vec2& pos, f32 depth);
    virtual void SetSize(const vec2& size);
    virtual vec2 GetPosition();
    virtual vec2 GetSize();
    virtual f32 GetDepth();

private:
    UI::Widget* _widget;
};