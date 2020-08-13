#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Collidable.h"

namespace UIScripting
{
    Panel::Panel() : BaseElement(UI::UIElementType::UITYPE_PANEL)
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
            registry->emplace<UIComponent::Image>(_entityId);
            registry->emplace<UIComponent::Renderable>(_entityId);
            registry->emplace<UIComponent::Collidable>(_entityId);

            UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
            events->asObject = this;
        }
        uiLockSingleton.mutex.unlock();
    }

    void Panel::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Panel", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Panel>("BaseElement");
        r = ScriptEngine::RegisterScriptFunction("Panel@ CreatePanel()", asFUNCTION(Panel::CreatePanel)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetEventFlag(int8 flags)", asMETHOD(Panel, SetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void UnsetEventFlag(int8 flags)", asMETHOD(Panel, UnsetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(Panel, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsDraggable()", asMETHOD(Panel, IsDraggable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(Panel, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void PanelEventCallback(Panel@ panel)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnClickCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnDragged(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnDragCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocused(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnFocusCallback)); assert(r >= 0);

        // Renderable Functions
        r = ScriptEngine::RegisterScriptClassFunction("string GetTexture()", asMETHOD(Panel, GetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string Texture)", asMETHOD(Panel, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(Panel, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Panel, SetColor)); assert(r >= 0);
    }

    const bool Panel::IsClickable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsClickable();
    }
    const bool Panel::IsDraggable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsDraggable();
    }
    const bool Panel::IsFocusable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsFocusable();
    }

    void Panel::SetEventFlag(const UI::UITransformEventsFlags flags)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->SetFlag(flags);
    }
    void Panel::UnsetEventFlag(const UI::UITransformEventsFlags flags)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->UnsetFlag(flags);
    }

    void Panel::SetOnClickCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onClickCallback = callback;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }
    void Panel::SetOnDragCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onDraggedCallback = callback;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
    }
    void Panel::SetOnFocusCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusedCallback = callback;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    const std::string& Panel::GetTexture() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->texture;
    }
    void Panel::SetTexture(const std::string& texture)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Image* image = &registry->get<UIComponent::Image>(_entityId);
        image->texture = texture;
    }

    const Color Panel::GetColor() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->color;

    }
    void Panel::SetColor(const Color& color)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Image* image = &registry->get<UIComponent::Image>(_entityId);
        image->color = color;
    }

    Panel* Panel::CreatePanel()
    {
        Panel* panel = new Panel();
        
        return panel;
    }
}