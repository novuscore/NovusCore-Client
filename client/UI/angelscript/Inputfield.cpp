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

        UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
        events->asObject = this;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        UIComponent::InputField* inputField = &registry->emplace<UIComponent::InputField>(_entityId);
        inputField->asObject = this;

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
            entt::registry* registry = ServiceLocator::GetUIRegistry();
            bool multiLine = &registry->get<UIComponent::Text>(_entityId).isMultiline;
            if (multiLine)
            {
                HandleCharInput('\n');
                break;
            }

            registry->get<UIComponent::InputField>(_entityId).OnSubmit();
            registry->get<UIComponent::TransformEvents>(_entityId).OnUnfocused();
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

        if (text->text.length() == inputField->writeHeadIndex)
            text->text += input;
        else
            text->text.insert(inputField->writeHeadIndex, 1, input);

        MovePointerRight();
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
            events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
        else
            events->UnsetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void InputField::SetOnFocusCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusedCallback = callback;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }
    void InputField::SetOnUnFocusCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onUnfocusedCallback = callback;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
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

    const Color& InputField::GetTextColor() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->style.color;
    }
    void InputField::SetTextColor(const Color& color)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->style.color = color;
    }

    const Color& InputField::GetTextOutlineColor() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->style.outlineColor;
    }
    void InputField::SetTextOutlineColor(const Color& outlineColor)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->style.outlineColor = outlineColor;
    }

    const f32 InputField::GetTextOutlineWidth() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->style.outlineWidth;
    }
    void InputField::SetTextOutlineWidth(f32 outlineWidth)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->style.outlineWidth = outlineWidth;
    }

    void InputField::SetTextFont(const std::string& fontPath, f32 fontSize)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->style.fontPath = fontPath;
        text->style.fontSize = fontSize;
    }

    InputField* InputField::CreateInputField()
    {
        InputField* inputField = new InputField();

        return inputField;
    }
}