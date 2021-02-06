#include "Inputfield.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "../ECS/Components/ElementInfo.h"
#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Text.h"
#include "../ECS/Components/InputField.h"
#include "../ECS/Components/Renderable.h"
#include "../Utils/TextUtils.h"
#include "../Utils/EventUtils.h"

#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>

namespace UIScripting
{
    InputField::InputField() : EventElement(UI::ElementType::UITYPE_INPUTFIELD, true, UI::TransformEventsFlags::FLAG_FOCUSABLE)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::InputField>(_entityId);
        registry->emplace<UIComponent::Text>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Text;
    }

    void InputField::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("InputField", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = RegisterEventBase<InputField>("InputField");
        r = ScriptEngine::RegisterScriptFunction("InputField@ CreateInputField()", asFUNCTION(InputField::CreateInputField)); assert(r >= 0);

        // InputField Functions
        r = ScriptEngine::RegisterScriptClassFunction("void OnSubmit(InputFieldEventCallback@ cb)", asMETHOD(InputField, SetOnSubmitCallback)); assert(r >= 0);

        //Text Functions
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(InputField, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text, bool updateWriteHead = true)", asMETHOD(InputField, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(TextStylesheet stylesheet)", asMETHOD(InputField, SetStylesheet)); assert(r >= 0);
    }

    bool InputField::OnKeyInput(i32 key)
    {
        switch (key)
        {
        case GLFW_KEY_BACKSPACE:
            RemovePreviousCharacter();
            break;
        case GLFW_KEY_DELETE:
            RemoveNextCharacter();
            break;
        case GLFW_KEY_RIGHT:
            MovePointerRight();
            break;
        case GLFW_KEY_LEFT:
            MovePointerLeft();
            break;
        case GLFW_KEY_ENTER:
        {
            entt::registry* registry = ServiceLocator::GetUIRegistry();
            if (registry->get<UIComponent::Text>(_entityId).style.multiline)
            {
                OnCharInput('\n');
                break;
            }
            auto [elementInfo, inputField, events] = registry->get<UIComponent::ElementInfo, UIComponent::InputField, UIComponent::TransformEvents>(_entityId);
            UIUtils::ExecuteEvent(elementInfo.scriptingObject, inputField.onSubmitCallback);
            UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onFocusLostCallback);

            registry->ctx<UISingleton::UIDataSingleton>().focusedElement = entt::null;
            events.UnsetState(UI::TransformEventState::STATE_FOCUSED);
            break;
        }
        default:
            return false;
        }
        MarkSelfDirty();

        return true;
    }

    bool InputField::OnCharInput(const char input)
    {
        auto [text, inputField] = ServiceLocator::GetUIRegistry()->get<UIComponent::Text, UIComponent::InputField>(_entityId);

        text.text.insert(inputField.writeHeadIndex, 1, input); 
        inputField.writeHeadIndex++;
        MarkSelfDirty();

        return true;
    }

    void InputField::RemovePreviousCharacter()
    {
        auto [text, inputField] = ServiceLocator::GetUIRegistry()->get<UIComponent::Text, UIComponent::InputField>(_entityId);

        if (text.text.empty() || inputField.writeHeadIndex == 0)
            return;

        inputField.writeHeadIndex--;
        text.text.erase(inputField.writeHeadIndex, 1);
    }
    void InputField::RemoveNextCharacter()
    {
        auto [text, inputField] = ServiceLocator::GetUIRegistry()->get<UIComponent::Text, UIComponent::InputField>(_entityId);

        if (text.text.empty())
            return;

        text.text.erase(inputField.writeHeadIndex, 1);
    }

    void InputField::MovePointerLeft()
    {
        UIComponent::InputField& inputField = ServiceLocator::GetUIRegistry()->get<UIComponent::InputField>(_entityId);
        if (inputField.writeHeadIndex > 0)
            inputField.writeHeadIndex--;
    }
    void InputField::MovePointerRight()
    {
        auto [text, inputField] = ServiceLocator::GetUIRegistry()->get<UIComponent::Text, UIComponent::InputField>(_entityId);

        if (inputField.writeHeadIndex < text.text.length())
            inputField.writeHeadIndex++;
    }
    void InputField::SetWriteHeadPosition(size_t position)
    {
        auto [text, inputField] = ServiceLocator::GetUIRegistry()->get<UIComponent::Text, UIComponent::InputField>(_entityId);

        inputField.writeHeadIndex = Math::Min(position, text.text.length());
    }

    void InputField::SetOnSubmitCallback(asIScriptFunction* callback)
    {
        UIComponent::InputField& inputField = ServiceLocator::GetUIRegistry()->get<UIComponent::InputField>(_entityId);
        inputField.onSubmitCallback = callback;
    }

    const std::string InputField::GetText() const
    {
        const UIComponent::Text& text = ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text.text;
    }
    void InputField::SetText(const std::string& newText, bool updateWriteHead)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text& text = registry->get<UIComponent::Text>(_entityId);
        text.text = newText;

        if (updateWriteHead)
        {
            UIComponent::InputField& inputField = registry->get<UIComponent::InputField>(_entityId);
            inputField.writeHeadIndex = newText.length() - 1;
        }
    }

    void InputField::SetStylesheet(const UI::TextStylesheet& textStylesheet)
    {
        UIComponent::Text& text = ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        text.style = textStylesheet;
    }

    InputField* InputField::CreateInputField()
    {
        InputField* inputField = new InputField();

        return inputField;
    }
}