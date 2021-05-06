#include "Checkbox.h"
#include "Panel.h"
#include "../../Utils/ServiceLocator.h"

#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/ImageEventStyles.h"
#include "../ECS/Components/Checkbox.h"
#include "../Utils/EventUtils.h"

namespace UIScripting
{
    Checkbox::Checkbox() : EventElement(UI::ElementType::UITYPE_CHECKBOX, true, UI::TransformEventsFlags::FLAG_CLICKABLE)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::Checkbox>(_entityId);
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::ImageEventStyles>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId, UI::RenderType::Image);

        _checkPanel = Panel::CreatePanel(false);
        InternalAddChild(_checkPanel);
        auto checkTransform = &registry->get<UIComponent::Transform>(_checkPanel->GetEntityId());
        checkTransform->SetFlag(UI::TransformFlags::FILL_PARENTSIZE);
    }

    void Checkbox::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Checkbox", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = RegisterEventBase<Checkbox>("Checkbox");
        r = ScriptEngine::RegisterScriptFunction("Checkbox@ CreateCheckbox()", asFUNCTION(Checkbox::CreateCheckbox)); assert(r >= 0);

        // Checkbox Functions
        r = ScriptEngine::RegisterScriptClassFunction("bool IsChecked()", asMETHOD(Checkbox, IsChecked)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetChecked(bool checked)", asMETHOD(Checkbox, SetChecked)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnChecked(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnCheckedCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnUnchecked(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnUncheckedCallback)); assert(r >= 0);

        // Rendering Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(ImageStylesheet styleSheet)", asMETHOD(Checkbox, SetStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckStylesheet(ImageStylesheet styleSheet)", asMETHOD(Checkbox, SetCheckStylesheet)); assert(r >= 0);
    }

    void Checkbox::SetStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_NORMAL] = styleSheet;
    }
    void Checkbox::SetFocusedStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_FOCUSED] = styleSheet;
    }
    void Checkbox::SetHoverStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_HOVERED] = styleSheet;
    }
    void Checkbox::SetPressedStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_PRESSED] = styleSheet;
    }
    void Checkbox::SetDisabledStylesheet(const UI::ImageStylesheet& stylesheet)
    {
        UIComponent::ImageEventStyles* imageStyles = &ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        imageStyles->styleMap[UI::TransformEventState::STATE_DISABLED] = stylesheet;
    }

    void Checkbox::SetCheckStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        _checkPanel->SetStylesheet(styleSheet);
    }

    void Checkbox::SetOnCheckedCallback(asIScriptFunction* callback)
    {
        UIComponent::Checkbox& checkBox = ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        checkBox.onChecked = callback;
    }
    void Checkbox::SetOnUncheckedCallback(asIScriptFunction* callback)
    {
        UIComponent::Checkbox& checkBox = ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        checkBox.onUnchecked = callback;
    }

    const bool Checkbox::IsChecked() const
    {
        const UIComponent::Checkbox& checkBox = ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        return checkBox.checked;
    }
    void Checkbox::SetChecked(bool checked)
    {
        UIComponent::Checkbox& checkBox = ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        checkBox.checked = checked;

        _checkPanel->SetVisible(checked);

        if (checked)
            UIUtils::ExecuteEvent(this, checkBox.onChecked);
        else
            UIUtils::ExecuteEvent(this, checkBox.onUnchecked);
    }

    void Checkbox::OnClick(vec2 mousePosition)
    {
        UIComponent::Checkbox& checkBox = ServiceLocator::GetUIRegistry()->get<UIComponent::Checkbox>(_entityId);
        checkBox.checked = !checkBox.checked;

        _checkPanel->SetVisible(checkBox.checked);

        if (checkBox.checked)
            UIUtils::ExecuteEvent(this, checkBox.onChecked);
        else
            UIUtils::ExecuteEvent(this, checkBox.onUnchecked);
    }
    bool Checkbox::OnKeyInput(i32 key)
    {
        if (key == GLFW_KEY_ENTER)
        {
            OnClick(vec2());
            return true;
        }

        return false;
    }

    Checkbox* Checkbox::CreateCheckbox()
    {
        Checkbox* checkbox = new Checkbox();
        return checkbox;
    }
}