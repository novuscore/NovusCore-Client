#include "asInputfield.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../UI/TextUtils.h"
#include "../../../UI/TextUtilsTransactions.h"

#include "../../../ECS/Components/Singletons/ScriptSingleton.h"

#include <GLFW/glfw3.h>

namespace UI
{
    asInputField::asInputField() : asUITransform(UIElementType::UITYPE_INPUTFIELD)
    {
        SetFocusable(true);
    }

    void asInputField::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("InputField", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<asUITransform, asInputField>("UITransform");
        r = ScriptEngine::RegisterScriptFunction("InputField@ CreateInputField()", asFUNCTION(asInputField::CreateInputField)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptFunctionDef("void InputFieldEventCallback(InputField@ inputfield)"); assert(r >= 0);

        // InputField Functions
        r = ScriptEngine::RegisterScriptClassFunction("void OnSubmit(InputFieldEventCallback@ cb)", asMETHOD(asInputField, SetOnSubmitCallback)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetFocusable(bool focusable)", asMETHOD(asInputField, SetFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(asInputField, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocus(InputFieldEventCallback@ cb)", asMETHOD(asInputField, SetOnFocusCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnLostFocus(InputFieldEventCallback@ cb)", asMETHOD(asInputField, SetOnUnFocusCallback)); assert(r >= 0);

        //Text Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text, bool updateWriteHead = true)", asMETHOD(asInputField, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(asInputField, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTextColor(Color color)", asMETHOD(asInputField, SetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetTextColor()", asMETHOD(asInputField, GetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(asInputField, SetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(asInputField, GetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(asInputField, SetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(asInputField, GetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(asInputField, SetTextFont)); assert(r >= 0);
    }

    void asInputField::HandleKeyInput(i32 key)
    {
        switch (key)
        {
        case GLFW_KEY_BACKSPACE:
            RemovePreviousCharacter();
            break;
        case GLFW_KEY_DELETE:
            RemoveNextCharacter();
            break;
        case GLFW_KEY_LEFT:
            MovePointerLeft();
            break;
        case GLFW_KEY_RIGHT:
            MovePointerRight();
            break;
        case GLFW_KEY_ENTER:
        {
            if (_text.textType == TextType::SINGLELINE)
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                entt::entity Id = GetEntityId();
                registry->get<UIInputField>(Id).OnSubmit();
                registry->get<UITransformEvents>(Id).OnUnfocused();

                registry->ctx<UI::UIDataSingleton>().focusedWidget = entt::null;
            }
            else
            {
                HandleCharInput('\n');
            }
            break;
        }
        default:
            break;
        }
    }

    void asInputField::HandleCharInput(const char input)
    {
        std::string newString = GetText();
        if (_inputField.writeHeadIndex == newString.length())
        {
            newString += input;
        }
        else
        {
            newString.insert(_inputField.writeHeadIndex, 1, input);
        }

        SetText(newString, false);
        MovePointerRight();
    }

    void asInputField::RemovePreviousCharacter()
    {
        if (GetText().empty() || _inputField.writeHeadIndex <= 0)
            return;

        std::string newString = GetText();
        newString.erase(_inputField.writeHeadIndex - 1, 1);

        SetText(newString, false);

        MovePointerLeft();
    }

    void asInputField::RemoveNextCharacter()
    {
        if (GetText().length() <= _inputField.writeHeadIndex)
            return;

        std::string newString = GetText();
        newString.erase(_inputField.writeHeadIndex, 1);

        SetText(newString, false);
    }

    void asInputField::MovePointerLeft()
    {
        if(_inputField.writeHeadIndex > 0)
            SetWriteHeadPosition(_inputField.writeHeadIndex - 1);
    }

    void asInputField::MovePointerRight()
    {
        SetWriteHeadPosition(_inputField.writeHeadIndex + 1);
    }

    void asInputField::SetWriteHeadPosition(size_t position)
    {
        size_t clampedPosition = position <= GetText().length() ? position : GetText().length();

        if (clampedPosition == _inputField.writeHeadIndex)
            return;

        _inputField.writeHeadIndex = clampedPosition;

        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, clampedPosition]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();

                UIInputField& inputField = uiRegistry->get<UIInputField>(entId);
                inputField.writeHeadIndex = clampedPosition;

                UIText& text = uiRegistry->get<UIText>(entId);
                text.pushback = UI::TextUtils::CalculatePushback(text, inputField.writeHeadIndex, 0.2f, uiRegistry->get<UITransform>(entId).size.x);
                NC_LOG_MESSAGE("Pointer is now: %d, Pushback is now: %d", clampedPosition, text.pushback);

                MarkDirty(uiRegistry, entId);
            });
    }

    void asInputField::SetOnSubmitCallback(asIScriptFunction* callback)
    {
        _inputField.onSubmitCallback = callback;

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                UIInputField& inputField = ServiceLocator::GetUIRegistry()->get<UIInputField>(entId);
                inputField.onSubmitCallback = callback;
            });
    }

    void asInputField::SetFocusable(bool focusable)
    {
        if (focusable)
            _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
        else
            _events.UnsetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([focusable, entId]()
            {
                UITransformEvents& events = ServiceLocator::GetUIRegistry()->get<UITransformEvents>(entId);

                if (focusable)
                    events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
                else
                    events.UnsetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });
    }

    void asInputField::SetOnFocusCallback(asIScriptFunction* callback)
    {
        _events.onFocusedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);

                events.onFocusedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });
    }

    void asInputField::SetOnUnFocusCallback(asIScriptFunction* callback)
    {
        _events.onUnfocusedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);

                events.onUnfocusedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });
    }

    void asInputField::SetText(const std::string& text, bool updateWriteHead)
    {
        _text.text = text;

        if (updateWriteHead)
            _inputField.writeHeadIndex = text.length() - 1;

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, text, updateWriteHead]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);
                UIInputField& inputField = uiRegistry->get<UIInputField>(entId);
                uiText.text = text;

                if (updateWriteHead)
                    inputField.writeHeadIndex = text.length() - 1;

                MarkDirty(uiRegistry, entId);
            });
    }

    void asInputField::SetTextColor(const Color& color)
    {
        _text.color = color;

        UI::TextUtils::Transactions::SetColorTransaction(_entityId, color);
    }

    void asInputField::SetTextOutlineColor(const Color& outlineColor)
    {
        _text.outlineColor = outlineColor;

        UI::TextUtils::Transactions::SetOutlineColorTransaction(_entityId, outlineColor);
    }

    void asInputField::SetTextOutlineWidth(f32 outlineWidth)
    {
        _text.outlineWidth = outlineWidth;

        UI::TextUtils::Transactions::SetOutlineWidthTransaction(_entityId, outlineWidth);
    }

    void asInputField::SetTextFont(const std::string& fontPath, f32 fontSize)
    {
        _text.fontPath = fontPath;

        UI::TextUtils::Transactions::SetFontTransaction(_entityId, fontPath, fontSize);
    }

    asInputField* asInputField::CreateInputField()
    {
        asInputField* inputField = new asInputField();

        return inputField;
    }
}