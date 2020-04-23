#include "InputField.h"
#include "Label.h"
#include "../../Rendering/UIElementRegistry.h"

namespace UI
{
    InputField::InputField(const vec2& pos, const vec2& size) : Widget(pos, size)
    {
        _label = new Label(pos, size);
        _label->SetParent(this);

        //TODO UIRegistry
    }

    void InputField::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("InputField", 0, asOBJ_REF | asOBJ_NOCOUNT);
        assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptInheritance<Widget, InputField>("Widget");
            r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(InputField, SetTexture)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(InputField, SetColor)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(InputField, SetText)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetTextColor(Color col)", asMETHOD(InputField, SetTextColor)); assert(r >= 0);

        }
    }

    void InputField::SetColor(const Color& color)
    {
        _color = color;
    }

    const std::string& InputField::GetText() const
    {
        return _label->GetText();
    }
    void InputField::SetText(std::string& text)
    {
        _label->SetText(text);
    }
    void InputField::SetFont(std::string& fontPath, f32 fontSize)
    {
        _label->SetFont(fontPath, fontSize);
    }
    void InputField::SetTextColor(const Color& color)
    {
        _label->SetColor(color);
    }
}