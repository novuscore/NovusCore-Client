#include "Checkbox.h"
#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include <GLFW/glfw3.h>

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Collidable.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/Checkbox.h"

namespace UIScripting
{
    Checkbox::Checkbox() : BaseElement(UI::UIElementType::UITYPE_CHECKBOX)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
        events->asObject = this;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);

        UIComponent::Checkbox* checkBox = &registry->emplace<UIComponent::Checkbox>(_entityId);
        checkBox->asObject = this;

        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;

        _checkPanel = Panel::CreatePanel(false);
        auto checkPanelTransform = &registry->get<UIComponent::Transform>(_checkPanel->GetEntityId());
        checkPanelTransform->parent = _entityId;
        checkPanelTransform->SetFlag(UI::TransformFlags::FILL_PARENTSIZE);
        registry->get<UIComponent::SortKey>(_checkPanel->GetEntityId()).data.depth++;
        registry->get<UIComponent::Transform>(_entityId).children.push_back({ _checkPanel->GetEntityId(), _checkPanel->GetType() });
    }

    void Checkbox::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Checkbox", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Checkbox>("BaseElement");
        r = ScriptEngine::RegisterScriptFunction("Checkbox@ CreateCheckbox()", asFUNCTION(Checkbox::CreateCheckbox)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetEventFlag(int8 flags)", asMETHOD(Checkbox, SetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void UnsetEventFlag(int8 flags)", asMETHOD(Checkbox, UnsetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(Checkbox, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsDraggable()", asMETHOD(Checkbox, IsDraggable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(Checkbox, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void CheckboxEventCallback(Checkbox@ checkbox)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnClickCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocused(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnFocusCallback)); assert(r >= 0);

        // Rendering Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetBackgroundTexture(string Texture)", asMETHOD(Checkbox, SetBackgroundTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetBackgroundTexture()", asMETHOD(Checkbox, GetBackgroundTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBackgroundColor(Color color)", asMETHOD(Checkbox, SetBackgroundColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetBackgroundColor()", asMETHOD(Checkbox, GetBackgroundColor)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckTexture(string Texture)", asMETHOD(Checkbox, SetCheckTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetCheckTexture()", asMETHOD(Checkbox, GetCheckTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckColor(Color color)", asMETHOD(Checkbox, SetCheckColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetCheckColor()", asMETHOD(Checkbox, GetCheckColor)); assert(r >= 0);

        // Checkbox Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetChecked(bool checked)", asMETHOD(Checkbox, SetChecked)); assert(r >= 0);
    }

    const bool Checkbox::IsClickable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsClickable();

    }
    const bool Checkbox::IsDraggable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsDraggable();
    }
    const bool Checkbox::IsFocusable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsFocusable();
    }

    void Checkbox::SetEventFlag(const UI::UITransformEventsFlags flags)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->SetFlag(flags);
    }
    void Checkbox::UnsetEventFlag(const UI::UITransformEventsFlags flags)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->UnsetFlag(flags);
    }

    void Checkbox::SetOnClickCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onClickCallback = callback;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }
    void Checkbox::SetOnFocusCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusedCallback = callback;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    const std::string& Checkbox::GetBackgroundTexture() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.texture;
    }
    void Checkbox::SetBackgroundTexture(const std::string& texture)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Image* image = &registry->get<UIComponent::Image>(_entityId);
        image->style.texture = texture;
    }

    const Color Checkbox::GetBackgroundColor() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.color;
    }
    void Checkbox::SetBackgroundColor(const Color& color)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Image* image = &registry->get<UIComponent::Image>(_entityId);
        image->style.color = color;
    }

    const std::string& Checkbox::GetCheckTexture() const
    {
        return _checkPanel->GetTexture();
    }
    void Checkbox::SetCheckTexture(const std::string& texture)
    {
        _checkPanel->SetTexture(texture);
    }

    const Color Checkbox::GetCheckColor() const
    {
        return _checkPanel->GetColor();
    }
    void Checkbox::SetCheckColor(const Color& color)
    {
        _checkPanel->SetColor(color);
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
            checkBox->OnChecked();
        else
            checkBox->OnUnchecked();
    }
    void Checkbox::ToggleChecked()
    {
        SetChecked(!IsChecked());
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