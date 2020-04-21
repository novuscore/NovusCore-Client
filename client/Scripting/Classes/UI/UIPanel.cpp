#include "UIPanel.h"
#include "../../ScriptEngine.h"
#include "../../../Rendering/UIElementRegistry.h"

UIPanel::UIPanel(const vec2& pos, const vec2& size)
    : _panel(pos, size), UIWidget(&_panel), _onClickCallback(nullptr)
{
    UIElementRegistry::Instance()->AddUIPanel(this);
}

void UIPanel::RegisterType()
{
    i32 r = ScriptEngine::RegisterScriptClass("UIPanel", 0, asOBJ_REF | asOBJ_NOCOUNT);
    assert(r >= 0);
    {
        r = ScriptEngine::RegisterScriptInheritance<UIWidget, UIPanel>("UIWidget");
        r = ScriptEngine::RegisterScriptFunction("UIPanel@ CreatePanel(vec2 pos = vec2(0, 0), vec2 size = vec2(100, 100))", asFUNCTION(UIPanel::CreatePanel)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(vec4 color)", asMETHOD(UIPanel, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(UIPanel, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetClickable(bool value)", asMETHOD(UIPanel, SetClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(UIPanel, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetDragable(bool value)", asMETHOD(UIPanel, SetDraggable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsDragable()", asMETHOD(UIPanel, IsDraggable)); assert(r >= 0);

        // Callback
        r = ScriptEngine::RegisterScriptFunctionDef("void OnPanelClickCallback(UIPanel@ panel)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(OnPanelClickCallback@ cb)", asMETHOD(UIPanel, SetOnClick)); assert(r >= 0);
    }
}

std::string UIPanel::GetTypeName()
{
    return "UIPanel";
}

void UIPanel::SetColor(const vec4& color)
{
    _panel.SetColor(Color(color.r, color.g, color.b, color.a));
}
void UIPanel::SetTexture(std::string& texture)
{
    _panel.SetTexture(texture);
}

bool UIPanel::IsClickable()
{
    return _panel.IsClickable();
}
void UIPanel::SetClickable(bool value)
{
    _panel.SetClickable(value);
}

bool UIPanel::IsDraggable()
{
    return _panel.IsDraggable();
}
void UIPanel::SetDraggable(bool value)
{
    _panel.SetDraggable(value);
}

void UIPanel::SetOnClick(asIScriptFunction* function)
{
    _onClickCallback = function;
}
void UIPanel::OnClick()
{
    if (!_onClickCallback)
        return;

    asIScriptContext* context = ScriptEngine::GetScriptContext();
    {
        context->Prepare(_onClickCallback);
        {
            context->SetArgObject(0, this);
        }
        context->Execute();
    }
}

UI::Panel* UIPanel::GetInternal()
{
    return &_panel;
}
UIPanel* UIPanel::CreatePanel(const vec2& pos, const vec2& size)
{
    UIPanel* panel = new UIPanel(pos, size);
    return panel;
}