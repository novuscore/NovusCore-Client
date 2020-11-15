#include "Inputfield.h"
#include "../../Scripting/ScriptEngine.h"
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

namespace UIScripting
{
    InputField::InputField() : BaseElement(UI::ElementType::UITYPE_INPUTFIELD)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        registry->emplace<UIComponent::InputField>(_entityId);
        registry->emplace<UIComponent::Text>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Text;
    }

    void InputField::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("InputField", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, InputField>("BaseElement");
        r = ScriptEngine::RegisterScriptFunction("InputField@ CreateInputField()", asFUNCTION(InputField::CreateInputField)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptFunctionDef("void InputFieldEventCallback(InputField@ inputfield)"); assert(r >= 0);

        // InputField Functions
        r = ScriptEngine::RegisterScriptClassFunction("void OnSubmit(InputFieldEventCallback@ cb)", asMETHOD(InputField, SetOnSubmitCallback)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(InputField, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFocusable(bool focusable)", asMETHOD(InputField, SetFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocusGained(InputFieldEventCallback@ cb)", asMETHOD(InputField, SetOnFocusGainedCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocusLost(InputFieldEventCallback@ cb)", asMETHOD(InputField, SetOnFocusLostCallback)); assert(r >= 0);

        //Text Functions
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(InputField, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text, bool updateWriteHead = true)", asMETHOD(InputField, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(InputField, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(InputField, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(InputField, GetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(InputField, SetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(InputField, GetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(InputField, SetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(InputField, SetFont)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("bool IsMultiline()", asMETHOD(InputField, IsMultiline)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetMultiline(bool multiline)", asMETHOD(InputField, SetMultiline)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void SetHorizontalAlignment(uint8 alignment)", asMETHOD(InputField, SetHorizontalAlignment)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetVerticalAlignment(uint8 alignment)", asMETHOD(InputField, SetVerticalAlignment)); assert(r >= 0);
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
        case GLFW_KEY_RIGHT:
            MovePointerRight();
            break;
        case GLFW_KEY_LEFT:
            MovePointerLeft();
            break;
        case GLFW_KEY_ENTER:
        {
            entt::registry* registry = ServiceLocator::GetUIRegistry();
            if (registry->get<UIComponent::Text>(_entityId).multiline)
            {
                HandleCharInput('\n');
                break;
            }

            auto [elementInfo, inputField, events] = registry->get<UIComponent::ElementInfo, UIComponent::InputField, UIComponent::TransformEvents>(_entityId);
            UIUtils::ExecuteEvent(elementInfo.scriptingObject, inputField.onSubmitCallback);
            UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onFocusLostCallback);

            registry->ctx<UISingleton::UIDataSingleton>().focusedWidget = entt::null;
            break;
        }
        default:
            break;
        }
        MarkSelfDirty();
    }

    void InputField::HandleCharInput(const char input)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        UIComponent::InputField* inputField = &registry->get<UIComponent::InputField>(_entityId);

        text->text.insert(inputField->writeHeadIndex, 1, input);

        // Move pointer one step to the right.
        inputField->writeHeadIndex++;
    }

    void InputField::RemovePreviousCharacter()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        UIComponent::InputField* inputField = &registry->get<UIComponent::InputField>(_entityId);

        if (text->text.empty() || inputField->writeHeadIndex == 0)
            return;

        text->text.erase(inputField->writeHeadIndex - 1, 1);
        inputField->writeHeadIndex--;
    }
    void InputField::RemoveNextCharacter()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        const UIComponent::InputField* inputField = &registry->get<UIComponent::InputField>(_entityId);

        if (text->text.empty() || inputField->writeHeadIndex == 0)
            return;

        text->text.erase(inputField->writeHeadIndex, 1);
    }

    void InputField::MovePointerLeft()
    {
        UIComponent::InputField* inputField = &ServiceLocator::GetUIRegistry()->get<UIComponent::InputField>(_entityId);
        if (inputField->writeHeadIndex > 0)
            inputField->writeHeadIndex--;
    }
    void InputField::MovePointerRight()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        const UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        UIComponent::InputField* inputField = &registry->get<UIComponent::InputField>(_entityId);

        if (inputField->writeHeadIndex < text->text.length())
            inputField->writeHeadIndex++;
    }
    void InputField::SetWriteHeadPosition(size_t position)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::InputField* inputField = &registry->get<UIComponent::InputField>(_entityId);
        const UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);

        inputField->writeHeadIndex = Math::Min(position, text->text.length());
    }

    void InputField::SetOnSubmitCallback(asIScriptFunction* callback)
    {
        UIComponent::InputField* inputField = &ServiceLocator::GetUIRegistry()->get<UIComponent::InputField>(_entityId);
        inputField->onSubmitCallback = callback;
    }

    const bool InputField::IsFocusable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsFocusable();
    }
    void InputField::SetFocusable(bool focusable)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);

        if (focusable)
            events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
        else
            events->UnsetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void InputField::SetOnFocusGainedCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusGainedCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }
    void InputField::SetOnFocusLostCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusLostCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    const std::string InputField::GetText() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->text;
    }
    void InputField::SetText(const std::string& newText, bool updateWriteHead)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->text = newText;

        if (updateWriteHead)
        {
            UIComponent::InputField* inputField = &registry->get<UIComponent::InputField>(_entityId);
            inputField->writeHeadIndex = newText.length() - 1;
        }
    }

    const Color& InputField::GetColor() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->style.color;
    }
    void InputField::SetColor(const Color& color)
    {
        UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        text->style.color = color;
    }

    const Color& InputField::GetOutlineColor() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->style.outlineColor;
    }
    void InputField::SetOutlineColor(const Color& outlineColor)
    {
        UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        text->style.outlineColor = outlineColor;
    }

    const f32 InputField::GetOutlineWidth() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->style.outlineWidth;
    }
    void InputField::SetOutlineWidth(f32 outlineWidth)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->style.outlineWidth = outlineWidth;
    }

    void InputField::SetFont(const std::string& fontPath, f32 fontSize)
    {
        UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        text->style.fontPath = fontPath;
        text->style.fontSize = fontSize;
    }

    bool InputField::IsMultiline()
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->multiline;
    }
    void InputField::SetMultiline(bool multiline)
    {
        UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        text->multiline = multiline;
    }

    void InputField::SetHorizontalAlignment(UI::TextHorizontalAlignment alignment)
    {
        UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        text->horizontalAlignment = alignment;
    }
    void InputField::SetVerticalAlignment(UI::TextVerticalAlignment alignment)
    {
        UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        text->verticalAlignment = alignment;
    }

    InputField* InputField::CreateInputField()
    {
        InputField* inputField = new InputField();

        return inputField;
    }
}