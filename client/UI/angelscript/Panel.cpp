#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/Renderable.h"

namespace UIScripting
{
    Panel::Panel(bool collisionEnabled) : BaseElement(UI::ElementType::UITYPE_PANEL, collisionEnabled)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::TransformEvents>(_entityId);
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;
    }

    void Panel::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Panel", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Panel>("BaseElement");
        r = ScriptEngine::RegisterScriptFunction("Panel@ CreatePanel(bool collisionEnabled = true)", asFUNCTION(Panel::CreatePanel)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(Panel, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetClickable(bool clickable)", asMETHOD(Panel, SetClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsDraggable()", asMETHOD(Panel, IsDraggable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetDraggable(bool draggable)", asMETHOD(Panel, SetDraggable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(Panel, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFocusable(bool focusable)", asMETHOD(Panel, SetFocusable)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptFunctionDef("void PanelEventCallback(Panel@ panel)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnClickCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnDragStarted(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnDragStartedCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnDragEnded(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnDragEndedCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocusGained(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnFocusGainedCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocusLost(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnFocusLostCallback)); assert(r >= 0);

        // Renderable Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(ImageStylesheet styleSheet)", asMETHOD(Panel, SetStylesheet)); assert(r >= 0);
    }

    const bool Panel::IsClickable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }
    void Panel::SetClickable(bool clickable)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        if (clickable)
            events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
        else
            events->UnsetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }
    const bool Panel::IsDraggable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
    }
    void Panel::SetDraggable(bool draggable)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        if (draggable)
            events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
        else
            events->UnsetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
    }
    const bool Panel::IsFocusable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }
    void Panel::SetFocusable(bool focusable)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        if (focusable)
            events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
        else
            events->UnsetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void Panel::SetOnClickCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onClickCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }

    void Panel::SetOnDragStartedCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onDragStartedCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
    }
    void Panel::SetOnDragEndedCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onDragEndedCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
    }

    void Panel::SetOnFocusGainedCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusGainedCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }
    void Panel::SetOnFocusLostCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusLostCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void Panel::SetStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style = styleSheet;
    }

    Panel* Panel::CreatePanel(bool collisionEnabled)
    {
        Panel* panel = new Panel(collisionEnabled);

        return panel;
    }
}