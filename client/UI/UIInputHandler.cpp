#include "UIInputHandler.h"
#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>
#include <entity/entity.hpp>
#include <entity/registry.hpp>

#include "../Utils/ServiceLocator.h"

#include "ECS/Components/Singletons/UIDataSingleton.h"
#include "ECS/Components/ElementInfo.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/TransformEvents.h"
#include "ECS/Components/SortKey.h"
#include "ECS/Components/Collision.h"
#include "ECS/Components/Collidable.h"
#include "ECS/Components/Visible.h"
#include "ECS/Components/NotCulled.h"

#include "Utils/ElementUtils.h"
#include "Utils/TransformUtils.h"
#include "Utils/ColllisionUtils.h"
#include "Utils/EventUtils.h"

#include "angelscript/EventElement.h"

namespace UIInput
{
    inline bool IsPointOutsideRect(const vec2 point, const vec2 min, const vec2 max) { return point.x < min.x || point.x > max.x || point.y < min.y || point.y > max.y; }

    inline void UnFocusElement(entt::registry* registry, UISingleton::UIDataSingleton& dataSingleton, const UIComponent::ElementInfo& elementInfo, UIComponent::TransformEvents& events)
    {
        UIUtils::MarkDirty(registry, dataSingleton.focusedElement);
        events.UnsetState(UI::TransformEventState::STATE_FOCUSED);
        dataSingleton.focusedElement = entt::null;
        UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onFocusLostCallback);
    }

    inline void UnHover(entt::registry* registry, UISingleton::UIDataSingleton& dataSingleton, const UIComponent::ElementInfo& elementInfo, UIComponent::TransformEvents& events)
    {
        UIUtils::MarkDirty(registry, dataSingleton.hoveredElement);
        events.UnsetState(UI::TransformEventState::STATE_HOVERED);
        dataSingleton.hoveredElement = entt::null;
        UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onHoverEndedCallback);
    }

    bool OnMouseClick(Window* window, std::shared_ptr<Keybind> keybind)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();
        const hvec2 mouse = UIUtils::Transform::WindowPositionToUIPosition(ServiceLocator::GetInputManager()->GetMousePosition());

        //Unfocus last focused widget.
        entt::entity lastFocusedWidget = dataSingleton.focusedElement;
        if (dataSingleton.focusedElement != entt::null)
        {
            auto [elementInfo, events] = registry->get<UIComponent::ElementInfo, UIComponent::TransformEvents>(dataSingleton.focusedElement);
            UnFocusElement(registry, dataSingleton, elementInfo, events);
        }

        // Handle release events.
        if (keybind->state == GLFW_RELEASE)
        {
            // Stop dragging.
            if (dataSingleton.draggedElement != entt::null)
            {
                auto [elementInfo, events] = registry->get<UIComponent::ElementInfo, UIComponent::TransformEvents>(dataSingleton.draggedElement);
                dataSingleton.draggedElement = entt::null;
                UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onDragEndedCallback);

                return true;
            }

            // Stop pressing.
            if (dataSingleton.pressedElement != entt::null)
            {
                registry->get<UIComponent::TransformEvents>(dataSingleton.pressedElement).UnsetState(UI::TransformEventState::STATE_PRESSED);
                UIUtils::MarkDirty(registry, dataSingleton.pressedElement);
                dataSingleton.pressedElement = entt::null;
            }
        }

        auto eventGroup = registry->group<>(entt::get<UIComponent::TransformEvents, UIComponent::ElementInfo, UIComponent::SortKey, UIComponent::Collision, UIComponent::Collidable, UIComponent::Visible, UIComponent::NotCulled>);
        eventGroup.sort<UIComponent::SortKey>([](const UIComponent::SortKey& first, const UIComponent::SortKey& second) { return first.key > second.key; });
        for (auto entity : eventGroup)
        {
            auto [events, elementInfo, collision] = eventGroup.get<UIComponent::TransformEvents, UIComponent::ElementInfo, UIComponent::Collision>(entity);

            // Check so mouse if within widget bounds.
            if (IsPointOutsideRect(mouse, collision.minBound, collision.maxBound))
                continue;

            // Don't interact with the last focused element directly. Reserving first click for unfocusing it but still block clicking through it.
            // Also check if we have any events we can actually call else exit out early.
            if (lastFocusedWidget == entity || !events.flags)
                return true;

            if (keybind->state == GLFW_PRESS)
            {
                if (events.HasFlag(UI::TransformEventsFlags::FLAG_DRAGGABLE))
                {
                    const UIComponent::Transform& transform = registry->get<UIComponent::Transform>(entity);
                    dataSingleton.draggedElement = entity;
                    dataSingleton.dragOffset = mouse - (transform.anchorPosition + transform.position);
                    UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onDragStartedCallback);
                }

                if (events.HasFlag(UI::TransformEventsFlags::FLAG_CLICKABLE))
                {
                    events.SetState(UI::TransformEventState::STATE_PRESSED);
                    dataSingleton.pressedElement = entity;
                    UIUtils::MarkDirty(registry, entity);
                }
            }
            else if (keybind->state == GLFW_RELEASE)
            {
                // Focus element.
                if (events.HasFlag(UI::TransformEventsFlags::FLAG_FOCUSABLE))
                {
                    UIUtils::MarkDirty(registry, entity);
                    events.SetState(UI::TransformEventState::STATE_FOCUSED);
                    dataSingleton.focusedElement = entity;
                    UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onFocusGainedCallback);
                }

                // Click element.
                if (events.HasFlag(UI::TransformEventsFlags::FLAG_CLICKABLE))
                {
                    reinterpret_cast<UIScripting::EventElement*>(elementInfo.scriptingObject)->OnClick(mouse);

                    UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onClickCallback);
                }
            }

            return true;
        }

        return false;
    }

    void OnMousePositionUpdate(Window* window, f32 x, f32 y)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();
        const hvec2 mouse = UIUtils::Transform::WindowPositionToUIPosition(hvec2(x, y));
        
        // Handle dragging.
        if (dataSingleton.draggedElement != entt::null)
        {
            auto [elementInfo, transform, events] = registry->get<UIComponent::ElementInfo, UIComponent::Transform, UIComponent::TransformEvents>(dataSingleton.draggedElement);

            hvec2 newPos = mouse - transform.anchorPosition - dataSingleton.dragOffset;
            if (events.HasFlag(UI::TransformEventsFlags::FLAG_DRAGLOCK_X))
                newPos.x = transform.position.x;
            else if (events.HasFlag(UI::TransformEventsFlags::FLAG_DRAGLOCK_Y))
                newPos.y = transform.position.y;

            transform.position = newPos;

            // Handle OnDrag(s)
            reinterpret_cast<UIScripting::EventElement*>(elementInfo.scriptingObject)->OnDrag();

            UIUtils::MarkDirty(registry, dataSingleton.draggedElement);
            UIUtils::MarkChildrenDirty(registry, dataSingleton.draggedElement);
            UIUtils::Transform::UpdateChildPositions(registry, dataSingleton.draggedElement);
            UIUtils::Collision::MarkBoundsDirty(registry, dataSingleton.draggedElement);
        }
        
        // Check if we are still hovering over element.
        if (dataSingleton.hoveredElement != entt::null)
        {
            UIComponent::Collision& hoveredCollision = registry->get<UIComponent::Collision>(dataSingleton.hoveredElement);
            if (IsPointOutsideRect(mouse, hoveredCollision.minBound, hoveredCollision.maxBound))
            {
                // Update eventstate of old hover.
                auto [oldElementInfo, oldEvents] = registry->get<UIComponent::ElementInfo, UIComponent::TransformEvents>(dataSingleton.hoveredElement);
                UnHover(registry, dataSingleton, oldElementInfo, oldEvents);
            }
        }

        // Handle hover.
        auto eventGroup = registry->group<>(entt::get<UIComponent::TransformEvents, UIComponent::ElementInfo, UIComponent::SortKey, UIComponent::Collision, UIComponent::Collidable, UIComponent::Visible, UIComponent::NotCulled>);
        eventGroup.sort<UIComponent::SortKey>([](const UIComponent::SortKey& first, const UIComponent::SortKey& second) { return first.key > second.key; });
        for (auto entity : eventGroup)
        {
            // Check if mouse is within widget bounds.
            const UIComponent::Collision& collision = eventGroup.get<UIComponent::Collision>(entity);
            if (IsPointOutsideRect(mouse, collision.minBound, collision.maxBound))
                continue;

            // Hovered widget hasn't changed.
            if (dataSingleton.hoveredElement == entity)
                break;
            
            // If it has then unhover the old one.
            if (dataSingleton.hoveredElement != entt::null)
            {
                auto [oldElementInfo, oldEvents] = registry->get<UIComponent::ElementInfo, UIComponent::TransformEvents>(dataSingleton.hoveredElement);
                UnHover(registry, dataSingleton, oldElementInfo, oldEvents);
            }

            dataSingleton.hoveredElement = entity;

            // Update eventstate of new hover.
            auto [events, elementInfo] = eventGroup.get<UIComponent::TransformEvents, UIComponent::ElementInfo>(entity);
            events.SetState(UI::TransformEventState::STATE_HOVERED);
            UIUtils::MarkDirty(registry, entity);
            UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onHoverStartedCallback);

            break;
        }
    }

    bool OnKeyboardInput(Window* window, i32 key, i32 action, i32 modifiers)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        if (dataSingleton.focusedElement == entt::null)
            return false;
        else if (action == GLFW_RELEASE) // We handle PRESS & REPEAT events.
            return true;
        auto [elementInfo, events] = registry->get<UIComponent::ElementInfo, UIComponent::TransformEvents>(dataSingleton.focusedElement);

        bool keyHandled = reinterpret_cast<UIScripting::EventElement*>(elementInfo.scriptingObject)->OnKeyInput(key);
        if (keyHandled)
            return true;

        if (key == GLFW_KEY_ENTER && events.HasFlag(UI::TransformEventsFlags::FLAG_CLICKABLE))
        {
            UIUtils::ExecuteEvent(elementInfo.scriptingObject, events.onClickCallback);
        }
        else if (key == GLFW_KEY_ESCAPE)
        {
            UnFocusElement(registry, dataSingleton, elementInfo, events);
        }

        return true;
    }

    bool OnCharInput(Window* window, u32 unicodeKey)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        if (dataSingleton.focusedElement == entt::null)
            return false;

        const UIComponent::ElementInfo& elementInfo = registry->get<UIComponent::ElementInfo>(dataSingleton.focusedElement);
        reinterpret_cast<UIScripting::EventElement*>(elementInfo.scriptingObject)->OnCharInput(unicodeKey);

        return true;
    }

    void RegisterCallbacks()
    {
        InputManager* inputManager = ServiceLocator::GetInputManager();
        inputManager->RegisterKeybind("UI Click Checker", GLFW_MOUSE_BUTTON_LEFT, KEYBIND_ACTION_CLICK, KEYBIND_MOD_ANY, std::bind(&OnMouseClick, std::placeholders::_1, std::placeholders::_2));
        inputManager->RegisterMousePositionCallback("UI Mouse Position Checker", std::bind(&OnMousePositionUpdate, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        inputManager->RegisterKeyboardInputCallback("UI Keyboard Input Checker"_h, std::bind(&OnKeyboardInput, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        inputManager->RegisterCharInputCallback("UI Char Input Checker"_h, std::bind(&OnCharInput, std::placeholders::_1, std::placeholders::_2));
    }
}