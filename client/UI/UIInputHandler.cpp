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
#include "Utils/TransformUtils.h"
#include "Utils/ColllisionUtils.h"

#include "angelscript/Inputfield.h"
#include "angelscript/Checkbox.h"
#include "angelscript/Slider.h"
#include "angelscript/SliderHandle.h"

namespace UIInput
{
    bool OnMouseClick(Window* window, std::shared_ptr<Keybind> keybind)
    {
        ZoneScoped;
        const hvec2 mouse = ServiceLocator::GetInputManager()->GetMousePosition();
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        //Unfocus last focused widget.
        entt::entity lastFocusedWidget = dataSingleton.focusedWidget;
        if (dataSingleton.focusedWidget != entt::null)
        {
            registry->get<UIComponent::TransformEvents>(dataSingleton.focusedWidget).OnUnfocused();
            dataSingleton.focusedWidget = entt::null;
        }

        if (dataSingleton.draggedWidget != entt::null && keybind->state == GLFW_RELEASE)
        {
            registry->get<UIComponent::TransformEvents>(dataSingleton.draggedWidget).OnDragEnded();
            dataSingleton.draggedWidget = entt::null;

            return true;
        }

        auto eventGroup = registry->group<>(entt::get<UIComponent::TransformEvents, UIComponent::ElementInfo, UIComponent::SortKey, UIComponent::Collision, UIComponent::Collidable, UIComponent::Visible>);
        eventGroup.sort<UIComponent::SortKey>([](const UIComponent::SortKey& first, const UIComponent::SortKey& second) { return first.key > second.key; });
        for (auto entity : eventGroup)
        {
            UIComponent::TransformEvents& events = eventGroup.get<UIComponent::TransformEvents>(entity);
            const UIComponent::ElementInfo& elementInfo = eventGroup.get<UIComponent::ElementInfo>(entity);
            const UIComponent::Collision& collision = eventGroup.get<UIComponent::Collision>(entity);

            // Check so mouse if within widget bounds.
            if (mouse.x < collision.minBound.x || mouse.x > collision.maxBound.x || mouse.y < collision.minBound.y || mouse.y > collision.maxBound.y)
                continue;

            // Don't interact with the last focused widget directly. Reserving first click for unfocusing it but still block clicking through it.
            // Also check if we have any events we can actually call else exit out early.
            if (lastFocusedWidget == entity || !events.flags)
                return true;

            if (keybind->state == GLFW_PRESS)
            {
                if (events.IsDraggable())
                {
                    const UIComponent::Transform& transform = registry->get<UIComponent::Transform>(entity);
                    dataSingleton.draggedWidget = entity;
                    dataSingleton.dragOffset = mouse - (transform.position + transform.localPosition);
                    events.OnDragStarted();
                }
            }
            else if (keybind->state == GLFW_RELEASE)
            {
                if (events.IsFocusable())
                {
                    dataSingleton.focusedWidget = entity;
                    events.OnFocused();
                }

                if (events.IsClickable())
                {
                    if (elementInfo.type == UI::ElementType::UITYPE_CHECKBOX)
                    {
                        UIScripting::Checkbox* checkBox = reinterpret_cast<UIScripting::Checkbox*>(events.asObject);
                        checkBox->ToggleChecked();
                    } 
                    else if(elementInfo.type == UI::ElementType::UITYPE_SLIDER)
                    {
                        UIScripting::Slider* slider = reinterpret_cast<UIScripting::Slider*>(events.asObject);
                        slider->OnClicked(mouse);
                    }
                    events.OnClick();
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
        if (dataSingleton.draggedWidget != entt::null)
        {
            const UIComponent::ElementInfo* elementInfo = &registry->get<UIComponent::ElementInfo>(dataSingleton.draggedWidget);
            auto transform = &registry->get<UIComponent::Transform>(dataSingleton.draggedWidget);
            auto events = &registry->get<UIComponent::TransformEvents>(dataSingleton.draggedWidget);

            if (transform->parent != entt::null)
            {
                hvec2 newLocalPos = hvec2(x, y) - transform->position - dataSingleton.dragOffset;
                if (events->dragLockX)
                    newLocalPos.x = transform->localPosition.x;
                else if (events->dragLockY)
                    newLocalPos.y = transform->localPosition.y;

                transform->localPosition = newLocalPos;
            }
            else
            {
                hvec2 newPos = hvec2(x, y) - dataSingleton.dragOffset;
                if (events->dragLockX)
                    newPos.x = transform->position.x;
                else if (events->dragLockY)
                    newPos.y = transform->position.y;

                transform->position = newPos;
            }

            // Handle OnDrag(s)
            if (elementInfo->type == UI::ElementType::UITYPE_SLIDERHANDLE)
            {
                UIScripting::SliderHandle* sliderHandle = reinterpret_cast<UIScripting::SliderHandle*>(events->asObject);
                sliderHandle->OnDragged();
            }

            UIUtils::Transform::UpdateChildTransforms(registry, transform);
            UIUtils::Transform::MarkDirty(registry, dataSingleton.draggedWidget);
            UIUtils::Transform::MarkChildrenDirty(registry, dataSingleton.draggedWidget);
            UIUtils::Collision::MarkBoundsDirty(registry, dataSingleton.draggedWidget);
        }

        // Handle hover.
        auto eventGroup = registry->group<>(entt::get<UIComponent::TransformEvents, UIComponent::SortKey, UIComponent::Collision, UIComponent::Collidable, UIComponent::Visible>);
        eventGroup.sort<UIComponent::SortKey>([](const UIComponent::SortKey& first, const UIComponent::SortKey& second) { return first.key > second.key; });
        for (auto entity : eventGroup)
        {
            if (dataSingleton.draggedWidget == entity)
                continue;

            const UIComponent::Collision& collision = eventGroup.get<UIComponent::Collision>(entity);
            // Check so mouse if within widget bounds.
            if (x < collision.minBound.x || x > collision.maxBound.x || y < collision.minBound.y || y > collision.maxBound.y)
                continue;

            // Hovered widget hasn't changed.
            if (dataSingleton.hoveredWidget == entity)
                break;
            dataSingleton.hoveredWidget = entity;

            // TODO Update EventState.

            break;
        }
    }

    bool OnKeyboardInput(Window* window, i32 key, i32 action, i32 modifiers)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        if (dataSingleton.focusedWidget == entt::null)
            return false;

        if (action == GLFW_RELEASE)
            return true;

        UIComponent::TransformEvents& events = registry->get<UIComponent::TransformEvents>(dataSingleton.focusedWidget);
        if (key == GLFW_KEY_ESCAPE)
        {
            events.OnUnfocused();
            dataSingleton.focusedWidget = entt::null;

            return true;
        }

        const UIComponent::ElementInfo& elementInfo = registry->get<UIComponent::ElementInfo>(dataSingleton.focusedWidget);
        switch (elementInfo.type)
        {
        case UI::ElementType::UITYPE_INPUTFIELD:
        {
            UIScripting::InputField* inputFieldAS = reinterpret_cast<UIScripting::InputField*>(events.asObject);
            inputFieldAS->HandleKeyInput(key);
            break;
        }
        case UI::ElementType::UITYPE_CHECKBOX:
        {
            UIScripting::Checkbox* checkBoxAS = reinterpret_cast<UIScripting::Checkbox*>(events.asObject);
            checkBoxAS->HandleKeyInput(key);
            break;
        }
        default:
            if (key == GLFW_KEY_ENTER && events.IsClickable())
            {
                events.OnClick();
            }
            break;
        }

        return true;
    }

    bool OnCharInput(Window* window, u32 unicodeKey)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        if (dataSingleton.focusedWidget == entt::null)
            return false;

        const UIComponent::ElementInfo& elementInfo = registry->get<UIComponent::ElementInfo>(dataSingleton.focusedWidget);
        const UIComponent::TransformEvents& events = registry->get<UIComponent::TransformEvents>(dataSingleton.focusedWidget);
        if (elementInfo.type == UI::ElementType::UITYPE_INPUTFIELD)
        {
            UIScripting::InputField* inputField = reinterpret_cast<UIScripting::InputField*>(events.asObject);
            inputField->HandleCharInput((char)unicodeKey);
            inputField->MarkSelfDirty();
        }

        return true;
    }

    void RegisterCallbacks()
    {
        InputManager* inputManager = ServiceLocator::GetInputManager();
        inputManager->RegisterKeybind("UI Click Checker", GLFW_MOUSE_BUTTON_LEFT, KEYBIND_ACTION_CLICK, KEYBIND_MOD_ANY, std::bind(&OnMouseClick, std::placeholders::_1, std::placeholders::_2));
        inputManager->RegisterMousePositionCallback("UI Mouse Position Checker", std::bind(&OnMousePositionUpdate, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        inputManager->RegisterKeyboardInputCallback("UI Keyboard Input Checker"_h, std::bind(&OnKeyboardInput, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        inputManager->RegisterCharInputCallback("UI Char Input Checker"_h, std::bind(&OnCharInput, std::placeholders::_1, std::placeholders::_2));

        // Create mouse group upfront. Reduces hitching from first mouse input.
        auto eventGroup = ServiceLocator::GetUIRegistry()->group<>(entt::get<UIComponent::TransformEvents, UIComponent::ElementInfo, UIComponent::SortKey, UIComponent::Collision, UIComponent::Collidable, UIComponent::Visible>);
    }
}