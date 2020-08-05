#include "Inputfield.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"
#include "../Utils/TextUtils.h"

#include <GLFW/glfw3.h>

namespace UIScripting
{
    InputField::InputField() : BaseElement(UI::UIElementType::UITYPE_INPUTFIELD)
    {
        SetFocusable(true);
    }

    void InputField::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("InputField", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, InputField>("Transform");
        r = ScriptEngine::RegisterScriptFunction("InputField@ CreateInputField()", asFUNCTION(InputField::CreateInputField)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptFunctionDef("void InputFieldEventCallback(InputField@ inputfield)"); assert(r >= 0);

        // InputField Functions
        r = ScriptEngine::RegisterScriptClassFunction("void OnSubmit(InputFieldEventCallback@ cb)", asMETHOD(InputField, SetOnSubmitCallback)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetFocusable(bool focusable)", asMETHOD(InputField, SetFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(InputField, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocus(InputFieldEventCallback@ cb)", asMETHOD(InputField, SetOnFocusCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnLostFocus(InputFieldEventCallback@ cb)", asMETHOD(InputField, SetOnUnFocusCallback)); assert(r >= 0);

        //Text Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text, bool updateWriteHead = true)", asMETHOD(InputField, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(InputField, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTextColor(Color color)", asMETHOD(InputField, SetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetTextColor()", asMETHOD(InputField, GetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(InputField, SetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(InputField, GetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(InputField, SetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(InputField, GetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(InputField, SetTextFont)); assert(r >= 0);
    }

    void InputField::HandleKeyInput(i32 key)
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
            if (_text.isMultiline)
            {
                HandleCharInput('\n');
                break;
            }

            entt::registry* registry = ServiceLocator::GetUIRegistry();
            registry->get<UIComponent::InputField>(_entityId).OnSubmit();
            registry->get<UIComponent::TransformEvents>(_entityId).OnUnfocused();
            registry->ctx<UISingleton::UIDataSingleton>().focusedWidget = entt::null;
            break;
        }
        default:
            break;
        }
    }

    void InputField::HandleCharInput(const char input)
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

    void InputField::RemovePreviousCharacter()
    {
        if (GetText().empty() || _inputField.writeHeadIndex <= 0)
            return;

        std::string newString = GetText();
        newString.erase(_inputField.writeHeadIndex - 1, 1);

        SetText(newString, false);

        MovePointerLeft();
    }

    void InputField::RemoveNextCharacter()
    {
        if (GetText().length() <= _inputField.writeHeadIndex)
            return;

        std::string newString = GetText();
        newString.erase(_inputField.writeHeadIndex, 1);

        SetText(newString, false);
    }

    void InputField::MovePointerLeft()
    {
        if (_inputField.writeHeadIndex > 0)
            SetWriteHeadPosition(_inputField.writeHeadIndex - 1);
    }

    void InputField::MovePointerRight()
    {
        SetWriteHeadPosition(_inputField.writeHeadIndex + 1);
    }

    void InputField::SetWriteHeadPosition(size_t position)
    {
        size_t clampedPosition = position <= GetText().length() ? position : GetText().length();

        if (clampedPosition == _inputField.writeHeadIndex)
            return;

        _inputField.writeHeadIndex = clampedPosition;

        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, clampedPosition]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();

                UIComponentUIInputField& inputField = uiRegistry->get<UIComponentUIInputField>(entId);
                inputField.writeHeadIndex = clampedPosition;

                Text& text = uiRegistry->get<Text>(entId);
                const Transform& transform = uiRegistry->get<Transform>(entId);
                text.pushback = UI::TextUtils::CalculatePushback(text, inputField.writeHeadIndex, 0.2f, transform.size.x, transform.size.y);
                NC_LOG_MESSAGE("Pointer is now: %d, Pushback is now: %d", clampedPosition, text.pushback);

                MarkDirty(uiRegistry, entId);
            });*/
    }

    void InputField::SetOnSubmitCallback(asIScriptFunction* callback)
    {
        _inputField.onSubmitCallback = callback;

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                UIComponentUIInputField& inputField = ServiceLocator::GetUIRegistry()->get<UIComponentUIInputField>(entId);
                inputField.onSubmitCallback = callback;
            });*/
    }

    void InputField::SetFocusable(bool focusable)
    {
        if (focusable)
            _events.SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
        else
            _events.UnsetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([focusable, entId]()
            {
                UIComponent::TransformEvents& events = ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(entId);

                if (focusable)
                    events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
                else
                    events.UnsetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });*/
    }

    void InputField::SetOnFocusCallback(asIScriptFunction* callback)
    {
        _events.onFocusedCallback = callback;
        _events.SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIComponent::TransformEvents& events = uiRegistry->get<UIComponent::TransformEvents>(entId);

                events.onFocusedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });*/
    }

    void InputField::SetOnUnFocusCallback(asIScriptFunction* callback)
    {
        _events.onUnfocusedCallback = callback;
        _events.SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIComponent::TransformEvents& events = uiRegistry->get<UIComponent::TransformEvents>(entId);

                events.onUnfocusedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });*/
    }

    void InputField::SetText(const std::string& text, bool updateWriteHead)
    {
        _text.text = text;

        if (updateWriteHead)
            _inputField.writeHeadIndex = text.length() - 1;

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, text, updateWriteHead]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                Text& uiText = uiRegistry->get<Text>(entId);
                UIComponentUIInputField& inputField = uiRegistry->get<UIComponentUIInputField>(entId);
                uiText.text = text;

                if (updateWriteHead)
                    inputField.writeHeadIndex = text.length() - 1;

                MarkDirty(uiRegistry, entId);
            });*/
    }

    void InputField::SetTextColor(const Color& color)
    {
        _text.color = color;

        //UI::TextUtils::Transactions::SetColorTransaction(_entityId, color);
    }

    void InputField::SetTextOutlineColor(const Color& outlineColor)
    {
        _text.outlineColor = outlineColor;

        //UI::TextUtils::Transactions::SetOutlineColorTransaction(_entityId, outlineColor);
    }

    void InputField::SetTextOutlineWidth(f32 outlineWidth)
    {
        _text.outlineWidth = outlineWidth;

        //UI::TextUtils::Transactions::SetOutlineWidthTransaction(_entityId, outlineWidth);
    }

    void InputField::SetTextFont(const std::string& fontPath, f32 fontSize)
    {
        _text.fontPath = fontPath;

        //UI::TextUtils::Transactions::SetFontTransaction(_entityId, fontPath, fontSize);
    }

    InputField* InputField::CreateInputField()
    {
        InputField* inputField = new InputField();

        return inputField;
    }
}