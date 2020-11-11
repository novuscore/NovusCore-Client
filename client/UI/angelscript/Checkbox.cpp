#include "Checkbox.h"
#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include <GLFW/glfw3.h>

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/SortKey.h"
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
        r = ScriptEngine::RegisterScriptClassFunction("string GetTexture()", asMETHOD(Checkbox, GetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string Texture)", asMETHOD(Checkbox, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(Checkbox, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Checkbox, SetColor)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("string GetBorder()", asMETHOD(Checkbox, GetBorder)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorder(string Texture)", asMETHOD(Checkbox, SetBorder)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderSize(uint topSize, uint rightSize, uint bottomSize, uint leftSize)", asMETHOD(Checkbox, SetBorderSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderInset(uint topBorderInset, uint rightBorderInset, uint bottomBorderInset, uint leftBorderInset)", asMETHOD(Checkbox, SetBorderInset)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetSlicing(uint topOffset, uint rightOffset, uint bottomOffset, uint leftOffset)", asMETHOD(Checkbox, SetSlicing)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("string GetCheckTexture()", asMETHOD(Checkbox, GetCheckTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckTexture(string Texture)", asMETHOD(Checkbox, SetCheckTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetCheckColor()", asMETHOD(Checkbox, GetCheckColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckColor(Color color)", asMETHOD(Checkbox, SetCheckColor)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("string GetCheckBorder()", asMETHOD(Checkbox, GetCheckBorder)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckBorder(string Texture)", asMETHOD(Checkbox, SetCheckBorder)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckBorderSize(uint topSize, uint rightSize, uint bottomSize, uint leftSize)", asMETHOD(Checkbox, SetCheckBorderSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckBorderInset(uint topBorderInset, uint rightBorderInset, uint bottomBorderInset, uint leftBorderInset)", asMETHOD(Checkbox, SetCheckBorderInset)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCheckSlicing(uint topOffset, uint rightOffset, uint bottomOffset, uint leftOffset)", asMETHOD(Checkbox, SetCheckSlicing)); assert(r >= 0);
    }

    const bool Checkbox::IsClickable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsClickable();

    }
    const bool Checkbox::IsFocusable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsFocusable();
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

    const std::string& Checkbox::GetTexture() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.texture;
    }
    void Checkbox::SetTexture(const std::string& texture)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Image* image = &registry->get<UIComponent::Image>(_entityId);
        image->style.texture = texture;
    }

    const Color Checkbox::GetColor() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.color;
    }
    void Checkbox::SetColor(const Color& color)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Image* image = &registry->get<UIComponent::Image>(_entityId);
        image->style.color = color;
    }

    const std::string& Checkbox::GetBorder() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.border;
    }
    void Checkbox::SetBorder(const std::string& texture)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.border = texture;
    }
    void Checkbox::SetBorderSize(const u32 topSize, const u32 rightSize, const u32 bottomSize, const u32 leftSize)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.borderSize.top = topSize;
        image->style.borderSize.right = rightSize;
        image->style.borderSize.bottom = bottomSize;
        image->style.borderSize.left = leftSize;
    }
    void Checkbox::SetBorderInset(const u32 topBorderInset, const u32 rightBorderInset, const u32 bottomBorderInset, const u32 leftBorderInset)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.borderInset.top = topBorderInset;
        image->style.borderInset.right = rightBorderInset;
        image->style.borderInset.bottom = bottomBorderInset;
        image->style.borderInset.left = leftBorderInset;
    }
    void Checkbox::SetSlicing(const u32 topOffset, const u32 rightOffset, const u32 bottomOffset, const u32 leftOffset)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.slicingOffset.top = topOffset;
        image->style.slicingOffset.right = rightOffset;
        image->style.slicingOffset.bottom = bottomOffset;
        image->style.slicingOffset.left = leftOffset;
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

    const std::string& Checkbox::GetCheckBorder() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_checkPanel->GetEntityId());
        return image->style.border;
    }
    void Checkbox::SetCheckBorder(const std::string& texture)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_checkPanel->GetEntityId());
        image->style.border = texture;
    }
    void Checkbox::SetCheckBorderSize(const u32 topSize, const u32 rightSize, const u32 bottomSize, const u32 leftSize)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_checkPanel->GetEntityId());
        image->style.borderSize.top = topSize;
        image->style.borderSize.right = rightSize;
        image->style.borderSize.bottom = bottomSize;
        image->style.borderSize.left = leftSize;
    }
    void Checkbox::SetCheckBorderInset(const u32 topBorderInset, const u32 rightBorderInset, const u32 bottomBorderInset, const u32 leftBorderInset)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_checkPanel->GetEntityId());
        image->style.borderInset.top = topBorderInset;
        image->style.borderInset.right = rightBorderInset;
        image->style.borderInset.bottom = bottomBorderInset;
        image->style.borderInset.left = leftBorderInset;
    }
    void Checkbox::SetCheckSlicing(const u32 topOffset, const u32 rightOffset, const u32 bottomOffset, const u32 leftOffset)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_checkPanel->GetEntityId());
        image->style.slicingOffset.top = topOffset;
        image->style.slicingOffset.right = rightOffset;
        image->style.slicingOffset.bottom = bottomOffset;
        image->style.slicingOffset.left = leftOffset;
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