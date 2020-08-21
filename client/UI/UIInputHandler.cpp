#include "UIInputHandler.h"
#include "../Utils/ServiceLocator.h"
#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>

#include "ECS/Components/Singletons/UIDataSingleton.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/TransformEvents.h"
#include "ECS/Components/Collidable.h"
#include "ECS/Components/Visible.h"

#include "angelscript/Inputfield.h"
#include "angelscript/Checkbox.h"

namespace UIInput
{
    bool OnMouseClick(Window* window, std::shared_ptr<Keybind> keybind)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        const vec2 mouse = ServiceLocator::GetInputManager()->GetMousePosition();

        //Unfocus last focused widget.
        entt::entity lastFocusedWidget = dataSingleton.focusedWidget;
        if (dataSingleton.focusedWidget != entt::null)
        {
            registry->get<UIComponent::TransformEvents>(dataSingleton.focusedWidget).OnUnfocused();
            dataSingleton.focusedWidget = entt::null;
        }

        auto eventGroup = registry->group<UIComponent::TransformEvents>(entt::get<UIComponent::Transform, UIComponent::Collidable, UIComponent::Visible>);
        eventGroup.sort<UIComponent::Transform>([](UIComponent::Transform& first, UIComponent::Transform& second) { return first.sortKey > second.sortKey; });
        for (auto entity : eventGroup)
        {
            const UIComponent::Transform& transform = eventGroup.get<UIComponent::Transform>(entity);
            UIComponent::TransformEvents& events = eventGroup.get<UIComponent::TransformEvents>(entity);

            // Check so mouse if within widget bounds.
            if (!((mouse.x > transform.minBound.x && mouse.x < transform.maxBound.x) && (mouse.y > transform.minBound.y && mouse.y < transform.maxBound.y)))
                continue;

            // Don't interact with the last focused widget directly again. The first click is reserved for unfocusing it. But we still need to block clicking through it.
            if (lastFocusedWidget == entity)
                return true;

            // Check if we have any events we can actually call else exit out early. It needs to still block clicking through though.
            if (!events.flags)
                return true;

            if (keybind->state == GLFW_PRESS)
            {
                if (events.IsDraggable())
                {
                    // TODO FEATURE: Dragging
                }
            }
            else
            {
                if (events.IsFocusable())
                {
                    dataSingleton.focusedWidget = entity;

                    events.OnFocused();
                }

                if (events.IsClickable())
                {
                    if (transform.sortData.type == UI::UIElementType::UITYPE_CHECKBOX)
                    {
                        UIScripting::Checkbox* checkBox = reinterpret_cast<UIScripting::Checkbox*>(transform.asObject);
                        checkBox->ToggleChecked();
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
        // TODO FEATURE: Handle Dragging
    }

    bool OnKeyboardInput(Window* window, i32 key, i32 action, i32 modifiers)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        if (dataSingleton.focusedWidget == entt::null || action != GLFW_RELEASE)
            return false;

        UIComponent::Transform& transform = registry->get<UIComponent::Transform>(dataSingleton.focusedWidget);
        UIComponent::TransformEvents& events = registry->get<UIComponent::TransformEvents>(dataSingleton.focusedWidget);

        if (key == GLFW_KEY_ESCAPE)
        {
            events.OnUnfocused();
            dataSingleton.focusedWidget = entt::null;

            return true;
        }

        switch (transform.sortData.type)
        {
        case UI::UIElementType::UITYPE_INPUTFIELD:
        {
            UIScripting::InputField* inputFieldAS = reinterpret_cast<UIScripting::InputField*>(transform.asObject);
            inputFieldAS->HandleKeyInput(key);
            break;
        }
        case UI::UIElementType::UITYPE_CHECKBOX:
        {
            UIScripting::Checkbox* checkBoxAS = reinterpret_cast<UIScripting::Checkbox*>(transform.asObject);
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
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        if (dataSingleton.focusedWidget == entt::null)
            return false;

        UIComponent::Transform& transform = registry->get<UIComponent::Transform>(dataSingleton.focusedWidget);
        if (transform.sortData.type == UI::UIElementType::UITYPE_INPUTFIELD)
        {
            UIScripting::InputField* inputField = reinterpret_cast<UIScripting::InputField*>(transform.asObject);
            inputField->HandleCharInput((char)unicodeKey);
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
    }
}