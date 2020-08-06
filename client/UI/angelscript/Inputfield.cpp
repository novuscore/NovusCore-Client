#include "Inputfield.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"
#include "../Utils/TextUtils.h"

#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Collidable.h"

#include <GLFW/glfw3.h>

namespace UIScripting
{
    InputField::InputField() : BaseElement(UI::UIElementType::UITYPE_INPUTFIELD)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UISingleton::UILockSingleton& uiLockSingleton = registry->ctx<UISingleton::UILockSingleton>();
        uiLockSingleton.mutex.lock();
        {
            UIComponent::Transform* transform = &registry->emplace<UIComponent::Transform>(_entityId);
            transform->sortData.entId = _entityId;
            transform->sortData.type = _elementType;
            transform->asObject = this;

            registry->emplace<UIComponent::Visible>(_entityId);
            registry->emplace<UIComponent::Visibility>(_entityId);
            registry->emplace<UIComponent::Text>(_entityId);
            UIComponent::InputField* inputField = &registry->emplace<UIComponent::InputField>(_entityId);
            inputField->asObject = this;

            registry->emplace<UIComponent::Renderable>(_entityId);
            registry->emplace<UIComponent::Collidable>(_entityId);

            UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
            events->asObject = this;
        }
        uiLockSingleton.mutex.unlock();

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
            if (_text->isMultiline)
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
        if (_inputField->writeHeadIndex == newString.length())
        {
            newString += input;
        }
        else
        {
            newString.insert(_inputField->writeHeadIndex, 1, input);
        }

        SetText(newString, false);
        MovePointerRight();
    }

    void InputField::RemovePreviousCharacter()
    {
        if (GetText().empty() || _inputField->writeHeadIndex <= 0)
            return;

        std::string newString = GetText();
        newString.erase(_inputField->writeHeadIndex - 1, 1);

        SetText(newString, false);

        MovePointerLeft();
    }

    void InputField::RemoveNextCharacter()
    {
        if (GetText().length() <= _inputField->writeHeadIndex)
            return;

        std::string newString = GetText();
        newString.erase(_inputField->writeHeadIndex, 1);

        SetText(newString, false);
    }

    void InputField::MovePointerLeft()
    {
        if (_inputField->writeHeadIndex > 0)
            SetWriteHeadPosition(_inputField->writeHeadIndex - 1);
    }

    void InputField::MovePointerRight()
    {
        SetWriteHeadPosition(_inputField->writeHeadIndex + 1);
    }

    void InputField::SetWriteHeadPosition(size_t position)
    {
        size_t clampedPosition = position <= GetText().length() ? position : GetText().length();

        if (clampedPosition == _inputField->writeHeadIndex)
            return;

        _inputField->writeHeadIndex = clampedPosition;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        _text->pushback = UIUtils::Text::CalculatePushback(_text, _inputField->writeHeadIndex, 0.2f, _transform->size.x, _transform->size.y);
        MarkDirty(uiRegistry, _entityId);
    }

    void InputField::SetOnSubmitCallback(asIScriptFunction* callback)
    {
        _inputField->onSubmitCallback = callback;
    }

    void InputField::SetFocusable(bool focusable)
    {
        if (focusable)
            _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
        else
            _events->UnsetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void InputField::SetOnFocusCallback(asIScriptFunction* callback)
    {
        _events->onFocusedCallback = callback;
        _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void InputField::SetOnUnFocusCallback(asIScriptFunction* callback)
    {
        _events->onUnfocusedCallback = callback;
        _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void InputField::SetText(const std::string& text, bool updateWriteHead)
    {
        _text->text = text;

        if (updateWriteHead)
            _inputField->writeHeadIndex = text.length() - 1;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        MarkDirty(uiRegistry, _entityId);
    }

    void InputField::SetTextColor(const Color& color)
    {
        _text->color = color;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void InputField::SetTextOutlineColor(const Color& outlineColor)
    {
        _text->outlineColor = outlineColor;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void InputField::SetTextOutlineWidth(f32 outlineWidth)
    {
        _text->outlineWidth = outlineWidth;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void InputField::SetTextFont(const std::string& fontPath, f32 fontSize)
    {
        _text->fontPath = fontPath;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    InputField* InputField::CreateInputField()
    {
        InputField* inputField = new InputField();

        return inputField;
    }
}