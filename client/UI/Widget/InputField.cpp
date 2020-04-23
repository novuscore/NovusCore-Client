#include "InputField.h"
#include "Label.h"
#include "../../Rendering/UIElementRegistry.h"
#include <Utils\DebugHandler.h>

namespace UI
{
    InputField::InputField(const vec2& pos, const vec2& size) : Widget(pos, size)
        , _color(1.0f,1.0f,1.0f, 1.0f), _pointerIndex(0), _onSubmitCallback(nullptr), _onEnterCallback(nullptr)
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

            r = ScriptEngine::RegisterScriptFunctionDef("void OnInputFieldCallback(InputField@ inputField)"); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void OnSubmit(OnInputFieldCallback@ cb)", asMETHOD(InputField, SetOnSubmit)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void OnEnter(OnInputFieldCallback@ cb)", asMETHOD(InputField, SetOnEnter)); assert(r >= 0);
        }
    }

    void InputField::SetColor(const Color& color)
    {
        _color = color;
    }

    void InputField::AddText(const std::string& text)
    {
        std::string newString = _label->GetText();
        if (_pointerIndex == _label->GetTextLength())
        {
            newString += text;
        }
        else
        {
            newString.insert((size_t)_pointerIndex, text);
        }

        _label->SetText(newString);

        _pointerIndex += (u32)text.length();
    }

    void InputField::RemovePreviousCharacter()
    {
        if (!_label->GetText().empty() && _pointerIndex > 0)
        {
            std::string newString = _label->GetText();
            newString.erase(_pointerIndex - 1, 1);

            _label->SetText(newString);

            MovePointerLeft();
        }
    }

    void InputField::RemoveNextCharacter()
    {
        if (_label->GetText().length() > _pointerIndex)
        {
            std::string newString = _label->GetText();
            newString.erase(_pointerIndex, 1);

            _label->SetText(newString);
        }
    }

    void InputField::MovePointerLeft()
    {
        _pointerIndex = _pointerIndex > 0 ? _pointerIndex - 1 : 0;
        NC_LOG_MESSAGE("PointerIndex: %d, Length: %d", _pointerIndex, _label->GetTextLength());
    }

    void InputField::MovePointerRight()
    {
        _pointerIndex = _pointerIndex + 1 > _label->GetTextLength() ? _pointerIndex : _pointerIndex + 1;
        NC_LOG_MESSAGE("PointerIndex: %d, Length: %d", _pointerIndex, _label->GetTextLength());
    }

    const std::string& InputField::GetText() const
    {
        return _label->GetText();
    }
    void InputField::SetText(const std::string& text)
    {
        _label->SetText(text);

        _pointerIndex = (u32)text.length();
    }
    void InputField::SetFont(const std::string& fontPath, f32 fontSize)
    {
        _label->SetFont(fontPath, fontSize);
    }
    void InputField::SetTextColor(const Color& color)
    {
        _label->SetColor(color);
    }
    
    void InputField::SetOnSubmit(asIScriptFunction* function)
    {
        _onSubmitCallback = function;
    }
    void InputField::OnSubmit()
    {
        if (!_onSubmitCallback)
            return;

        asIScriptContext* context = ScriptEngine::GetScriptContext();
        {
            context->Prepare(_onSubmitCallback);
            {
                context->SetArgObject(0, this);
            }
            context->Execute();
        }
    }

    void InputField::SetOnEnter(asIScriptFunction* function)
    {
        _onEnterCallback = function;
    }

    void InputField::OnEnter()
    {
        if (!_onEnterCallback)
            return;

        asIScriptContext* context = ScriptEngine::GetScriptContext();
        {
            context->Prepare(_onEnterCallback);
            {
                context->SetArgObject(0, this);
            }
            context->Execute();
        }
    }
    
    InputField* InputField::CreateInputField(const vec2& pos, const vec2& size)
    {
        InputField* inputField = new InputField(pos, size);
        return inputField;
    }
}