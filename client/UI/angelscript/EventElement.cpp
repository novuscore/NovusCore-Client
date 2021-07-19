#include "EventElement.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/TransformEvents.h"

namespace UIScripting
{
    EventElement::EventElement(UI::ElementType elementType, const std::string& name, bool collisionEnabled, u8 eventFlags) : BaseElement(elementType, name, collisionEnabled)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::TransformEvents& events = registry->emplace<UIComponent::TransformEvents>(_entityId);
        events.flags = eventFlags;
    }

    void EventElement::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("EventElement", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, EventElement>("BaseElement"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void EventElementEventCallback(EventElement@ element)"); assert(r >= 0);
        RegisterBase<EventElement>();
    }

    const bool EventElement::IsClickable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasFlag(UI::TransformEventsFlags::FLAG_CLICKABLE);
    }
    void EventElement::SetClickable(bool clickable)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        if (clickable)
            events->SetFlag(UI::TransformEventsFlags::FLAG_CLICKABLE);
        else
            events->UnsetFlag(UI::TransformEventsFlags::FLAG_CLICKABLE);
    }
    
    const bool EventElement::IsDraggable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasFlag(UI::TransformEventsFlags::FLAG_DRAGGABLE);
    }
    void EventElement::SetDraggable(bool draggable)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        if (draggable)
            events->SetFlag(UI::TransformEventsFlags::FLAG_DRAGGABLE);
        else
            events->UnsetFlag(UI::TransformEventsFlags::FLAG_DRAGGABLE);
    }
    
    const bool EventElement::IsFocusable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasFlag(UI::TransformEventsFlags::FLAG_FOCUSABLE);
    }
    void EventElement::SetFocusable(bool focusable)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        if (focusable)
            events->SetFlag(UI::TransformEventsFlags::FLAG_FOCUSABLE);
        else
            events->UnsetFlag(UI::TransformEventsFlags::FLAG_FOCUSABLE);
    }

    const bool EventElement::IsEnabled() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasState(UI::TransformEventState::STATE_DISABLED);
    }
    void EventElement::SetEnabled(bool enabled)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        if (enabled)
            events->UnsetState(UI::TransformEventState::STATE_DISABLED);
        else
            events->SetState(UI::TransformEventState::STATE_DISABLED);

    }
    
    void EventElement::SetOnClickCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onClickCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::FLAG_CLICKABLE);
    }
    
    void EventElement::SetOnDragStartedCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onDragStartedCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::FLAG_DRAGGABLE);
    }
    void EventElement::SetOnDragEndedCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onDragEndedCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::FLAG_DRAGGABLE);
    }
    
    void EventElement::SetOnFocusGainedCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusGainedCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::FLAG_FOCUSABLE);
    }
    void EventElement::SetOnFocusLostCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onFocusLostCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::FLAG_FOCUSABLE);
    }
}
