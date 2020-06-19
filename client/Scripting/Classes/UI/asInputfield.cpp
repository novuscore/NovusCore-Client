#include "asInputfield.h"
#include "asLabel.h"
#include "asPanel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/UI/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/UIAddElementQueueSingleton.h"

namespace UI
{
    asInputField::asInputField(entt::entity entityId) : asUITransform(entityId, UIElementData::UIElementType::UITYPE_INPUTFIELD)
    {

    }

    void asInputField::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("InputField", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<asUITransform, asInputField>("UITransform");
        r = ScriptEngine::RegisterScriptFunction("InputField@ CreateInputField()", asFUNCTION(asInputField::CreateInputField)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(asInputField, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void InputFieldEventCallback(InputField@ inputfield)"); assert(r >= 0);
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

    void asInputField::AppendInput(const std::string& Input)
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
        _events.onUnFocusedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);

                events.onUnFocusedCallback = callback;
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
                uiText.isDirty = true;
            });

        if(updateWriteHead)
            SetWriteHeadPosition(static_cast<u32>(text.length()));
    }
    const std::string& asInputField::GetText() const
    {
        return _text.text;
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
                uiText.isDirty = true;
            });
    }
    const Color& asInputField::GetTextColor() const
    {
        return _text.color;
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
                uiText.isDirty = true;
            });
    }
    const Color& asInputField::GetTextOutlineColor() const
    {
        return _text.outlineColor;
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
                uiText.isDirty = true;
            });
    }
    const f32 asInputField::GetTextOutlineWidth() const
    {
        return _text.outlineWidth;
    }

    void asInputField::SetTextFont(std::string fontPath, f32 fontSize)
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
                uiText.isDirty = true;
            });
    }

    asInputField* asInputField::CreateInputField()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIEntityPoolSingleton& entityPool = registry->ctx<UIEntityPoolSingleton>();
        UIAddElementQueueSingleton& addElementQueue = registry->ctx<UIAddElementQueueSingleton>();

        UIElementData elementData;
        entityPool.entityIdPool.try_dequeue(elementData.entityId);
        elementData.type = UIElementData::UIElementType::UITYPE_INPUTFIELD;

        asInputField* inputfield = new asInputField(elementData.entityId);

        elementData.asObject = inputfield;

        addElementQueue.elementPool.enqueue(elementData);
        return inputfield;
    }
}