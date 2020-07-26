#include "UIInputSystem.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../input-lib/InputManager.h"
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>

#include "../../Components/UI/Singletons/UIDataSingleton.h"
#include "../../Components/UI/UITransform.h"
#include "../../Components/UI/UITransformEvents.h"
#include "../../Components/UI/UICollidable.h"
#include "../../Components/UI/UIVisible.h"

#include "../../../Scripting/Classes/UI/asInputfield.h"
#include "../../../Scripting/Classes/UI/asCheckbox.h"

namespace UI::InputSystem
{
    bool OnMouseClick(Window* window, std::shared_ptr<Keybind> keybind)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UI::UIDataSingleton& dataSingleton = registry->ctx<UI::UIDataSingleton>();

        const vec2 mouse = ServiceLocator::GetInputManager()->GetMousePosition();

        //Unfocus last focused widget.
        entt::entity lastFocusedWidget = dataSingleton.focusedWidget;
        if (dataSingleton.focusedWidget != entt::null)
        {
            registry->get<UITransformEvents>(dataSingleton.focusedWidget).OnUnfocused();

            dataSingleton.focusedWidget = entt::null;
        }

        auto eventGroup = registry->group<UITransformEvents>(entt::get<UITransform, UICollidable, UIVisible>);
        eventGroup.sort<UITransform>([](const UITransform& left, const UITransform& right) { return left.sortKey > right.sortKey; });
        for (auto entity : eventGroup)
        {
            const UITransform& transform = eventGroup.get<UITransform>(entity);
            UITransformEvents& events = eventGroup.get<UITransformEvents>(entity);

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
                    events.OnClick();

                    if (transform.sortData.type == UI::UIElementType::UITYPE_CHECKBOX)
                    {
                        UI::asCheckbox* checkBox = reinterpret_cast<UI::asCheckbox*>(transform.asObject);
                        checkBox->ToggleChecked();
                    }
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
        UI::UIDataSingleton& dataSingleton = registry->ctx<UI::UIDataSingleton>();

        if (dataSingleton.focusedWidget == entt::null || action == GLFW_PRESS)
            return false;

        UITransform& transform = registry->get<UITransform>(dataSingleton.focusedWidget);
        UITransformEvents& events = registry->get<UITransformEvents>(dataSingleton.focusedWidget);

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
            UI::asInputField* inputFieldAS = reinterpret_cast<UI::asInputField*>(transform.asObject);
            inputFieldAS->HandleKeyInput(key);
            break;
        }
        case UI::UIElementType::UITYPE_CHECKBOX:
        {
            UI::asCheckbox* checkBoxAS = reinterpret_cast<UI::asCheckbox*>(transform.asObject);
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
        UI::UIDataSingleton& dataSingleton = registry->ctx<UI::UIDataSingleton>();

        if (dataSingleton.focusedWidget == entt::null)
            return false;

        UITransform& transform = registry->get<UITransform>(dataSingleton.focusedWidget);
        if (transform.sortData.type == UI::UIElementType::UITYPE_INPUTFIELD)
        {
            UI::asInputField* inputField = reinterpret_cast<UI::asInputField*>(transform.asObject);
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