#include "UIButton.h"
#include "../../ScriptEngine.h"
#include "../../../Rendering/UIElementRegistry.h"

UIButton::UIButton(const vec2& pos, const vec2& size)
    : _button(pos, size), UIWidget(&_button), _onClickCallback(nullptr)
{
    UIElementRegistry::Instance()->AddUIButton(this);
}

void UIButton::RegisterType()
{
    i32 r = ScriptEngine::RegisterScriptClass("UIButton", 0, asOBJ_REF | asOBJ_NOCOUNT);
    assert(r >= 0);
    {
        r = ScriptEngine::RegisterScriptInheritance<UIWidget, UIButton>("UIWidget");
        r = ScriptEngine::RegisterScriptFunction("UIButton@ CreateButton(vec2 pos = vec2(0, 0), vec2 size = vec2(100, 100))", asFUNCTION(UIButton::CreateButton)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(UIButton, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(vec4 color)", asMETHOD(UIButton, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(UIButton, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetClickable(bool value)", asMETHOD(UIButton, SetClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(UIButton, IsClickable)); assert(r >= 0);

        // Callback
        r = ScriptEngine::RegisterScriptFunctionDef("void OnButtonClickCallback(UIButton@ button)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(OnButtonClickCallback@ cb)", asMETHOD(UIButton, SetOnClick)); assert(r >= 0);
    }
}

std::string UIButton::GetTypeName()
{
    return "UIButton";
}

void UIButton::SetText(std::string& text)
{
    _button.SetText(text);
}

void UIButton::SetColor(const vec4& color)
{
    _button.SetColor(Color(color.r, color.g, color.b, color.a));
}

void UIButton::SetTexture(std::string& texture)
{
    _button.SetTexture(texture);
}

bool UIButton::IsClickable()
{
    return _button.IsClickable();
}

void UIButton::SetClickable(bool value)
{
    _button.SetClickable(value);
}

void UIButton::SetOnClick(asIScriptFunction* function)
{
    _onClickCallback = function;
}

void UIButton::OnClick()
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

UIButton* UIButton::CreateButton(const vec2& pos, const vec2& size)
{
    UIButton* button = new UIButton(pos, size);
    return button;
}