#include "asInputfield.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"

#include "../../../ECS/Components/UI/Singletons/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"

#include "../../../ECS/Components/UI/UIDirty.h"

#include <GLFW/glfw3.h>

namespace UI
{
    asInputField::asInputField(entt::entity entityId) : asUITransform(entityId, UIElementType::UITYPE_INPUTFIELD) 
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
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(asInputField, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocus(InputFieldEventCallback@ cb)", asMETHOD(asInputField, SetOnFocusCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnLostFocus(InputFieldEventCallback@ cb)", asMETHOD(asInputField, SetOnUnFocusCallback)); assert(r >= 0);

        //Label Functions
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
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UI::UIDataSingleton& dataSingleton = registry->ctx<UI::UIDataSingleton>();

        UITransform& transform = registry->get<UITransform>(dataSingleton.focusedWidget);
        UIInputField& inputField = registry->get<UIInputField>(dataSingleton.focusedWidget);
        UITransformEvents& events = registry->get<UITransformEvents>(dataSingleton.focusedWidget);

        UI::asInputField* inputFieldAS = reinterpret_cast<UI::asInputField*>(transform.asObject);

        switch (key)
        {
        case GLFW_KEY_BACKSPACE:
            inputFieldAS->RemovePreviousCharacter();
            break;
        case GLFW_KEY_DELETE:
            inputFieldAS->RemoveNextCharacter();
            break;
        case GLFW_KEY_LEFT:
            inputFieldAS->MovePointerLeft();
            break;
        case GLFW_KEY_RIGHT:
            inputFieldAS->MovePointerRight();
            break;
        case GLFW_KEY_ENTER:
            inputField.OnSubmit();
            events.OnUnfocused();

            dataSingleton.focusedWidget = entt::null;
            break;
        default:
            break;
        }
    }

    void asInputField::HandleInput(const std::string& Input)
    {
        std::string newString = GetText();
        if (_inputField.writeHeadIndex == newString.length())
        {
            newString += Input;
        }
        else
        {
            newString.insert(_inputField.writeHeadIndex, Input);
        }

        SetText(newString, false);

        SetWriteHeadPosition(_inputField.writeHeadIndex + static_cast<u32>(Input.length()));
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
        SetWriteHeadPosition(_inputField.writeHeadIndex - 1);
    }

    void asInputField::MovePointerRight()
    {
        SetWriteHeadPosition(_inputField.writeHeadIndex + 1);
    }

    void asInputField::SetWriteHeadPosition(u32 position)
    {
        u32 clampedPosition = position <= GetText().length() ? position : static_cast<u32>(GetText().length());

        if (clampedPosition == _inputField.writeHeadIndex)
            return;

        _inputField.writeHeadIndex = clampedPosition;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([entId, clampedPosition]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIInputField& inputField = uiRegistry->get<UIInputField>(entId);

                inputField.writeHeadIndex = clampedPosition;
            });
    }

    void asInputField::SetOnSubmitCallback(asIScriptFunction* callback)
    {
        _inputField.onSubmitCallback = callback;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIInputField& inputField = uiRegistry->get<UIInputField>(entId);

                inputField.onSubmitCallback = callback;
            });
    }

    void asInputField::SetFocusable(bool focusable)
    {
        if (focusable)
            _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
        else
            _events.UnsetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([focusable, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);

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

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
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

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
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

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([text, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.text = text;

                MarkDirty(uiRegistry, entId)
            });

        if(updateWriteHead)
            SetWriteHeadPosition(static_cast<u32>(text.length()));
    }

    void asInputField::SetTextColor(const Color& color)
    {
        _text.color = color;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([color, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.color = color;
                MarkDirty(uiRegistry, entId)
            });
    }

    void asInputField::SetTextOutlineColor(const Color& outlineColor)
    {
        _text.outlineColor = outlineColor;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([outlineColor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.outlineColor = outlineColor;
                MarkDirty(uiRegistry, entId)
            });
    }

    void asInputField::SetTextOutlineWidth(f32 outlineWidth)
    {
        _text.outlineWidth = outlineWidth;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([outlineWidth, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.outlineWidth = outlineWidth;
                MarkDirty(uiRegistry, entId)
            });
    }

    void asInputField::SetTextFont(const std::string& fontPath, f32 fontSize)
    {
        _text.fontPath = fontPath;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([fontPath, fontSize, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.fontPath = fontPath;
                uiText.fontSize = fontSize;
                MarkDirty(uiRegistry, entId)
            });
    }

    asInputField* asInputField::CreateInputField()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIEntityPoolSingleton& entityPool = registry->ctx<UIEntityPoolSingleton>();

        asInputField* inputField = new asInputField(entityPool.DeQeueueId());

        return inputField;
    }
}