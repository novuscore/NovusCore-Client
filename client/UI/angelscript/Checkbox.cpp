#include "Checkbox.h"
#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include <GLFW/glfw3.h>

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/Checkbox.h"
#include "../Utils/EventUtils.h"

namespace UIScripting
{
    Checkbox::Checkbox() : BaseElement(UI::ElementType::UITYPE_CHECKBOX)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);

        registry->emplace<UIComponent::Checkbox>(_entityId);
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;

        _checkPanel = Panel::CreatePanel(false);
        InternalAddChild(_checkPanel);
        auto checkTransform = &registry->get<UIComponent::Transform>(_checkPanel->GetEntityId());
        checkTransform->SetFlag(UI::TransformFlags::FILL_PARENTSIZE);
    }

    void Checkbox::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Checkbox", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Checkbox>("BaseElement");
        r = ScriptEngine::RegisterScriptFunction("Checkbox@ CreateCheckbox()", asFUNCTION(Checkbox::CreateCheckbox)); assert(r >= 0);

        // Checkbox Functions
        r = ScriptEngine::RegisterScriptFunctionDef("void CheckboxEventCallback(Checkbox@ checkbox)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsChecked()", asMETHOD(Checkbox, IsChecked)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetChecked(bool checked)", asMETHOD(Checkbox, SetChecked)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnChecked(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnCheckedCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnUnchecked(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnUncheckedCallback)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(Checkbox, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(Checkbox, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnClickCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocusGained(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnFocusGainedCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocusLost(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnFocusLostCallback)); assert(r >= 0);

        // Rendering Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(ImageStylesheet styleSheet)", asMETHOD(Checkbox, SetStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckStylesheet(ImageStylesheet styleSheet)", asMETHOD(Checkbox, SetCheckStylesheet)); assert(r >= 0);
    }

    const bool Checkbox::IsClickable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);

    }
    const bool Checkbox::IsFocusable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }

    void Checkbox::SetOnClickCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onClickCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }
    void Checkbox::SetOnFocusGainedCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusGainedCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }
    void Checkbox::SetOnFocusLostCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusLostCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void Checkbox::SetStylesheet(UI::ImageStylesheet styleSheet)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style = styleSheet;
    }

    void Checkbox::SetCheckStylesheet(UI::ImageStylesheet styleSheet)
    {
        _checkPanel->SetStylesheet(styleSheet);
    }

    void Checkbox::SetOnCheckedCallback(asIScriptFunction* callback)
    {
        UIComponent::Checkbox* checkBox = &ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        checkBox->onChecked = callback;
    }
    void Checkbox::SetOnUncheckedCallback(asIScriptFunction* callback)
    {
        UIComponent::Checkbox* checkBox = &ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        checkBox->onUnchecked = callback;
    }

    const bool Checkbox::IsChecked() const
    {
        const UIComponent::Checkbox* checkBox = &ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        return checkBox->checked;
    }
    void Checkbox::SetChecked(bool checked)
    {
        UIComponent::Checkbox* checkBox = &ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        checkBox->checked = checked;

        _checkPanel->SetVisible(checked);

        if (checked)
            UIUtils::ExecuteEvent(this, checkBox->onChecked);
        else
            UIUtils::ExecuteEvent(this, checkBox->onUnchecked);
    }
    void Checkbox::ToggleChecked()
    {
        UIComponent::Checkbox* checkBox = &ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        checkBox->checked = !checkBox->checked;

        _checkPanel->SetVisible(checkBox->checked);

        if (checkBox->checked)
            UIUtils::ExecuteEvent(this, checkBox->onChecked);
        else
            UIUtils::ExecuteEvent(this, checkBox->onUnchecked);
    }

    void Checkbox::HandleKeyInput(i32 key)
    {
        if (key == GLFW_KEY_ENTER)
        {
            ToggleChecked();
        }
    }

    Checkbox* Checkbox::CreateCheckbox()
    {
        Checkbox* checkbox = new Checkbox();
        
        return checkbox;
    }
}