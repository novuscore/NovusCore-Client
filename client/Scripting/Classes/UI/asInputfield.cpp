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
        _label = asLabel::CreateLabel();
        _label->SetParent(this);
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
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(asInputField, SetText)); assert(r >= 0);
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
        std::string newString = _label->GetText();
        if (_inputField.writeHeadIndex == _label->GetText().length())
        {
            newString += Input;
        }
        else
        {
            newString.insert(_inputField.writeHeadIndex, Input);
        }

        _label->SetText(newString);

        SetWriteHeadPosition(_inputField.writeHeadIndex + static_cast<u32>(Input.length()));
    }

    void asInputField::RemovePreviousCharacter()
    {
        if (_label->GetText().empty() || _inputField.writeHeadIndex <= 0)
            return;

        std::string newString = _label->GetText();
        newString.erase(_inputField.writeHeadIndex - 1, 1);

        _label->SetText(newString);

        MovePointerLeft();
    }

    void asInputField::RemoveNextCharacter()
    {
        if (_label->GetText().length() <= _inputField.writeHeadIndex)
            return;

        std::string newString = _label->GetText();
        newString.erase(_inputField.writeHeadIndex, 1);

        _label->SetText(newString);
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
        u32 clampedPosition = position > 0 ? (position <= _label->GetText().length() ? position : static_cast<u32>(_label->GetText().length())) : 0;

        if (clampedPosition == position)
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

    void asInputField::SetSize(const vec2& size)
    {
        asUITransform::SetSize(size);

        //This should be replaced by a system for filling parent size.
        _label->SetSize(size);
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

    void asInputField::SetText(const std::string& text)
    {
        _label->SetText(text);

        SetWriteHeadPosition(static_cast<u32>(_label->GetText().length()));
    }
    const std::string& asInputField::GetText() const
    {
        return _label->GetText();
    }

    void asInputField::SetTextColor(const Color& color)
    {
        _label->SetColor(color);
    }
    const Color& asInputField::GetTextColor() const
    {
        return _label->GetColor();
    }

    void asInputField::SetTextOutlineColor(const Color& outlineColor)
    {
        _label->SetOutlineColor(outlineColor);
    }
    const Color& asInputField::GetTextOutlineColor() const
    {
        return _label->GetOutlineColor();
    }

    void asInputField::SetTextOutlineWidth(f32 outlineWidth)
    {
        _label->SetOutlineWidth(outlineWidth);
    }
    const f32 asInputField::GetTextOutlineWidth() const
    {
        return _label->GetOutlineWidth();
    }

    void asInputField::SetTextFont(std::string fontPath, f32 fontSize)
    {
        _label->SetFont(fontPath, fontSize);
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