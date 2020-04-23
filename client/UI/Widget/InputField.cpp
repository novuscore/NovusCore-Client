#include "InputField.h"
#include "Label.h"
#include "../../Rendering/UIElementRegistry.h"

namespace UI
{
    InputField::InputField(const vec2& pos, const vec2& size) : Widget(pos, size)
    {
        _label = new Label(pos, size);
        _label->SetParent(this);

        UIElementRegistry::Instance()->AddInputField(this);
    }

    void InputField::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("InputField", 0, asOBJ_REF | asOBJ_NOCOUNT);
        assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptInheritance<Widget, InputField>("Widget");
            r = ScriptEngine::RegisterScriptFunction("InputField@ CreateInputField(vec2 pos = vec2(0, 0), vec2 size = vec2(100, 100))", asFUNCTION(InputField::CreateInputField)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(InputField, SetTexture)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(InputField, SetColor)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(InputField, SetFont)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(InputField, SetText)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetTextColor(Color col)", asMETHOD(InputField, SetTextColor)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(InputField, GetText)); assert(r >= 0);
        }
    }

    void InputField::SetColor(const Color& color)
    {
        _color = color;
    }

    void InputField::AddText(const std::string& character)
    {
        _label->SetText(_label->GetText() + character);
    }

    void InputField::RemoveCharacter()
    {
        _label->SetText(_label->GetText().substr(0, _label->GetText().length()-1));
    }

    const std::string& InputField::GetText() const
    {
        return _label->GetText();
    }
    void InputField::SetText(const std::string& text)
    {
        _label->SetText(text);
    }
    void InputField::SetFont(const std::string& fontPath, f32 fontSize)
    {
        _label->SetFont(fontPath, fontSize);
    }
    void InputField::SetTextColor(const Color& color)
    {
        _label->SetColor(color);
    }
    InputField* InputField::CreateInputField(const vec2& pos, const vec2& size)
    {
        InputField* inputField = new InputField(pos, size);
        return inputField;
    }
}