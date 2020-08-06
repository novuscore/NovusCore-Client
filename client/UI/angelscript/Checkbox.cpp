#include "Checkbox.h"
#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include <GLFW/glfw3.h>

#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Collidable.h"

namespace UIScripting
{
    Checkbox::Checkbox() : BaseElement(UI::UIElementType::UITYPE_CHECKBOX)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UISingleton::UILockSingleton& uiLockSingleton = registry->ctx<UISingleton::UILockSingleton>();
        uiLockSingleton.mutex.lock();
        {
            _transform = &registry->emplace<UIComponent::Transform>(_entityId);
            _transform->sortData.entId = _entityId;
            _transform->sortData.type = _elementType;
            _transform->asObject = this;

            registry->emplace<UIComponent::Visible>(_entityId);
            _visibility = &registry->emplace<UIComponent::Visibility>(_entityId);
            _image = &registry->emplace<UIComponent::Image>(_entityId);
            _checkBox = &registry->emplace<UIComponent::Checkbox>(_entityId);
            _checkBox->asObject = this;

            registry->emplace<UIComponent::Renderable>(_entityId);
            registry->emplace<UIComponent::Collidable>(_entityId);

            _events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
            _events->asObject = this;
        }
        uiLockSingleton.mutex.unlock();

        checkPanel = Panel::CreatePanel();
        checkPanel->SetFillParentSize(true);
        checkPanel->SetParent(this);
        checkPanel->SetCollisionEnabled(false);

        SetEventFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }

    void Checkbox::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Checkbox", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Checkbox>("Transform");
        r = ScriptEngine::RegisterScriptFunction("Checkbox@ CreateCheckbox()", asFUNCTION(Checkbox::CreateCheckbox)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetEventFlag(int8 flags)", asMETHOD(Checkbox, SetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void UnsetEventFlag(int8 flags)", asMETHOD(Checkbox, UnsetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(Checkbox, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsDraggable()", asMETHOD(Checkbox, IsDraggable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(Checkbox, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void CheckboxEventCallback(Checkbox@ checkbox)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnClickCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnDragged(CheckboxEventCallback@ cb)", asMETHOD(Checkbox, SetOnDragCallback)); assert(r >= 0);
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

    void Checkbox::SetEventFlag(const UI::UITransformEventsFlags flags)
    {
        _events->SetFlag(flags);
    }

    void Checkbox::UnsetEventFlag(const UI::UITransformEventsFlags flags)
    {
        _events->UnsetFlag(flags);
    }

    void Checkbox::SetOnClickCallback(asIScriptFunction* callback)
    {
        _events->onClickCallback = callback;
        _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }

    void Checkbox::SetOnDragCallback(asIScriptFunction* callback)
    {
        _events->onDraggedCallback = callback;
        _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
    }

    void Checkbox::SetOnFocusCallback(asIScriptFunction* callback)
    {
        _events->onFocusedCallback = callback;
        _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void Checkbox::SetBackgroundTexture(const std::string& texture)
    {
        _image->texture = texture;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }
    void Checkbox::SetBackgroundColor(const Color& color)
    {
        _image->color = color;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void Checkbox::SetCheckTexture(const std::string& texture)
    {
        checkPanel->SetTexture(texture);
    }

    const std::string& Checkbox::GetCheckTexture() const
    {
        return checkPanel->GetTexture();
    }

    void Checkbox::SetCheckColor(const Color& color)
    {
        checkPanel->SetColor(color);
    }

    const Color Checkbox::GetCheckColor() const
    {
        return checkPanel->GetColor();
    }

    void Checkbox::ToggleChecked()
    {
        SetChecked(!IsChecked());
    }

    void Checkbox::SetChecked(bool checked)
    {
        _checkBox->checked = checked;

        checkPanel->SetVisible(checked);

        if (checked)
            _checkBox->OnChecked();
        else
            _checkBox->OnUnchecked();
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