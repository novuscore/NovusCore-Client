#include "Button.h"
#include "Label.h"
#include "../../Rendering/UIElementRegistry.h"

namespace UI
{
    // Public
    Button::Button(const vec2& pos, const vec2& size)
        : Widget(pos, size)
        , _color(1.0f, 1.0f, 1.0f, 1.0f)
        , _clickable(true)
        , _onClickCallback(nullptr)
    {
        _label = new Label(pos, size);
        _label->SetParent(this);

        UIElementRegistry::Instance()->AddButton(this);
    }

    void Button::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Button", 0, asOBJ_REF | asOBJ_NOCOUNT);
        assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptInheritance<Widget, Button>("Widget");
            r = ScriptEngine::RegisterScriptFunction("Button@ CreateButton(vec2 pos = vec2(0, 0), vec2 size = vec2(100, 100))", asFUNCTION(Button::CreateButton)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(Button, SetTexture)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Button, SetColor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(Button, SetText)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetTextColor(Color col)", asMETHOD(Button, SetTextColor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(Button, SetFont)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetClickable(bool value)", asMETHOD(Button, SetClickable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(Button, IsClickable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("Label@ GetLabel()", asMETHOD(Button, GetLabel)); assert(r >= 0);

            // Callback
            r = ScriptEngine::RegisterScriptFunctionDef("void OnButtonClickCallback(Button@ button)"); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void OnClick(OnButtonClickCallback@ cb)", asMETHOD(Button, SetOnClick)); assert(r >= 0);
        }
    }

    // Public
    void Button::SetColor(const Color& color)
    {
        _color = color;
    }

    void Button::SetClickable(bool value)
    {
        _clickable = value;
    }

    const std::string& Button::GetText() const
    {
        return _label->GetText();
    }
    void Button::SetText(std::string& text)
    {
        _label->SetText(text);
    }

    void Button::SetFont(std::string& fontPath, f32 fontSize)
    {
        _label->SetFont(fontPath, fontSize);
    }

    void Button::SetTextColor(const Color& color)
    {
        _label->SetColor(color);
    }

    void Button::SetOnClick(asIScriptFunction* function)
    {
        _onClickCallback = function;
    }

    void Button::OnClick()
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

    //Private
    Button* Button::CreateButton(const vec2& pos, const vec2& size)
    {
        Button* button = new Button(pos, size);
        return button;
    }
}