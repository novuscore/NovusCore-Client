#include "InputManager.h"
#include <Utils/StringUtils.h>
#include <GLFW/glfw3.h>

InputManager::InputManager() : _keyToKeybindMap(), _titleToKeybindMap(), _keyboardInputCallbackMap(), _charInputCallbackMap()
{
    // Mouse
    _keyToKeybindMap[GLFW_MOUSE_BUTTON_1] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_MOUSE_BUTTON_2] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_MOUSE_BUTTON_3] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_MOUSE_BUTTON_4] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_MOUSE_BUTTON_5] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_MOUSE_BUTTON_6] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_MOUSE_BUTTON_7] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_MOUSE_BUTTON_8] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();

    // Keyboard
    _keyToKeybindMap[GLFW_KEY_SPACE] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_APOSTROPHE] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_COMMA] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_MINUS] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_PERIOD] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_SLASH] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_0] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_1] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_2] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_3] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_4] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_5] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_6] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_7] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_8] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_9] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_SEMICOLON] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_EQUAL] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_A] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_B] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_C] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_D] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_E] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_G] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_H] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_I] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_J] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_K] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_L] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_M] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_N] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_O] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_P] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_Q] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_R] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_S] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_T] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_U] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_V] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_W] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_X] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_Y] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_Z] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_LEFT_BRACKET] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_BACKSLASH] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_RIGHT_BRACKET] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_GRAVE_ACCENT] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_WORLD_1] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_WORLD_2] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_ESCAPE] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_ENTER] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_TAB] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_BACKSPACE] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_INSERT] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_DELETE] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_RIGHT] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_LEFT] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_DOWN] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_UP] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_PAGE_UP] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_PAGE_DOWN] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_HOME] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_END] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_CAPS_LOCK] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_SCROLL_LOCK] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_NUM_LOCK] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_PRINT_SCREEN] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_PAUSE] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F1] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F2] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F3] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F4] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F5] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F6] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F7] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F8] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F9] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F10] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F11] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F12] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F13] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F14] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F15] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F16] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F17] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F18] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F19] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F20] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F21] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F22] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F23] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F24] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_F25] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_0] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_1] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_2] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_3] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_4] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_5] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_6] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_7] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_8] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_9] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_DECIMAL] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_DIVIDE] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_MULTIPLY] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_SUBTRACT] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_ADD] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_ENTER] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_KP_EQUAL] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_LEFT_SHIFT] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_LEFT_CONTROL] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_LEFT_ALT] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_LEFT_SUPER] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_RIGHT_SHIFT] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_RIGHT_CONTROL] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_RIGHT_ALT] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_RIGHT_SUPER] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();
    _keyToKeybindMap[GLFW_KEY_MENU] = robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>();

    // Micro Optimization (Pre Allocate Memory for 8 InputBindings)
    for (auto inputBinding : _keyToKeybindMap)
    {
        inputBinding.second.reserve(8);
    }

    _titleToKeybindMap.reserve(64);
    _keyboardInputCallbackMap.reserve(8);
}

void InputManager::KeyboardInputHandler(Window* window, i32 key, i32 /*scanCode*/, i32 actionMask, i32 modifierMask)
{
    for (auto kv : _keyboardInputCallbackMap)
    {
        //If this returns true it consumed the input.
        if (kv.second(window, key, actionMask, modifierMask))
            return;
    }

    for (auto kv : _keyToKeybindMap[key])
    {
        auto inputBinding = kv.second;

        // We always want to update the state of the keybind on release as we cannot be certain that the keybind has bound release as an action
        if (actionMask == GLFW_RELEASE)
            inputBinding->state = 0;

        // Validate ActionMask and then check Modifier Mask
        bool validModifier = inputBinding->modifierMask == KEYBIND_MOD_ANY || inputBinding->modifierMask & modifierMask || inputBinding->modifierMask == 0 && modifierMask == 0;
        if ((inputBinding->actionMask & (1 << actionMask)) && validModifier)
        {
            inputBinding->state = actionMask == GLFW_RELEASE ? 0 : 1;

            if (!inputBinding->callback)
                continue;

            //If this returns true it consumed the input.
            if (inputBinding->callback(window, inputBinding))
                return;
        }
    }
}
void InputManager::CharInputHandler(Window* window, u32 unicodeKey)
{
    for (auto kv : _charInputCallbackMap)
    {
        //If this returns true it consumed the input.
        if (kv.second(window, unicodeKey))
            break;
    }
}
void InputManager::MouseInputHandler(Window* window, i32 button, i32 actionMask, i32 modifierMask)
{
    _mouseState = actionMask == GLFW_RELEASE ? 0 : 1;

    for (auto kv : _keyToKeybindMap[button])
    {
        auto inputBinding = kv.second;

        // Validate ActionMask and then check Modifier Mask
        bool validModifier = inputBinding->modifierMask == KEYBIND_MOD_ANY || inputBinding->modifierMask & modifierMask || inputBinding->modifierMask == 0 && modifierMask == 0;
        if ((inputBinding->actionMask & (1 << actionMask)) && validModifier)
        {
            inputBinding->state = _mouseState;

            if (!inputBinding->callback)
                continue;

            inputBinding->callback(window, inputBinding);
        }
    }
}
void InputManager::MousePositionHandler(Window* window, f32 x, f32 y)
{
    _mousePositionX = x;
    _mousePositionY = y;

    for (auto kv : _mousePositionUpdateCallbacks)
    {
        kv.second(window, x, y);
    }
}

bool InputManager::RegisterKeybind(std::string keybindTitle, i32 key, i32 actionMask, i32 modifierMask, std::function<KeybindCallbackFunc> callback)
{
    u32 keybindTitleHash = StringUtils::fnv1a_32(keybindTitle.c_str(), keybindTitle.length());

    for (auto kv : _keyToKeybindMap)
    {
        auto iterator = kv.second.find(keybindTitleHash);
        if (iterator != kv.second.end())
            return false;
    }

    std::shared_ptr<Keybind> keybind = std::make_shared<Keybind>(keybindTitle, actionMask, key, modifierMask, callback);
    _keyToKeybindMap[key][keybindTitleHash] = keybind;
    _titleToKeybindMap[keybindTitleHash] = keybind;

    return true;
}

bool InputManager::UnregisterKeybind(std::string keybindTitle)
{
    u32 keybindTitleHash = StringUtils::fnv1a_32(keybindTitle.c_str(), keybindTitle.length());

    auto iterator = _titleToKeybindMap.find(keybindTitleHash);
    if (iterator == _titleToKeybindMap.end())
        return false;

    i32 key = _titleToKeybindMap[keybindTitleHash]->key;
    _keyToKeybindMap[key].erase(keybindTitleHash);
    _titleToKeybindMap.erase(keybindTitleHash);
    return true;
}

bool InputManager::RegisterKeyboardInputCallback(u32 callbackNameHash, std::function<KeyboardInputCallbackFunc> callback)
{
    auto iterator = _keyboardInputCallbackMap.find(callbackNameHash);
    if (iterator != _keyboardInputCallbackMap.end())
        return false;

    _keyboardInputCallbackMap[callbackNameHash] = callback;
    return true;
}

bool InputManager::UnregisterKeyboardInputCallback(u32 callbackNameHash)
{
    auto iterator = _keyboardInputCallbackMap.find(callbackNameHash);
    if (iterator == _keyboardInputCallbackMap.end())
        return false;

    _keyboardInputCallbackMap.erase(callbackNameHash);
    return true;
}

bool InputManager::RegisterCharInputCallback(u32 callbackNameHash, std::function<CharInputCallbackFunc> callback)
{
    auto iterator = _charInputCallbackMap.find(callbackNameHash);
    if (iterator != _charInputCallbackMap.end())
        return false;

    _charInputCallbackMap[callbackNameHash] = callback;
    return true;
}

bool InputManager::UnregisterCharInputCallback(u32 callbackNameHash)
{
    auto iterator = _charInputCallbackMap.find(callbackNameHash);
    if (iterator == _charInputCallbackMap.end())
        return false;

    _charInputCallbackMap.erase(callbackNameHash);
    return true;
}

bool InputManager::RegisterMousePositionCallback(std::string callbackName, std::function<MousePositionUpdateFunc> callback)
{
    u32 hashedCallbackName = StringUtils::fnv1a_32(callbackName.c_str(), callbackName.length());

    auto iterator = _mousePositionUpdateCallbacks.find(hashedCallbackName);
    if (iterator != _mousePositionUpdateCallbacks.end())
        return false;

    _mousePositionUpdateCallbacks[hashedCallbackName] = callback;
    return true;
}

bool InputManager::UnregisterMousePositionCallback(std::string callbackName)
{
    u32 hashedCallbackName = StringUtils::fnv1a_32(callbackName.c_str(), callbackName.length());

    auto iterator = _mousePositionUpdateCallbacks.find(hashedCallbackName);
    if (iterator == _mousePositionUpdateCallbacks.end())
        return false;

    _mousePositionUpdateCallbacks.erase(hashedCallbackName);
    return true;
}

bool InputManager::GetKeybind(std::string keybindTitle, std::shared_ptr<Keybind>& keybind)
{
    u32 keybindTitleHash = StringUtils::fnv1a_32(keybindTitle.c_str(), keybindTitle.length());

    auto iterator = _titleToKeybindMap.find(keybindTitleHash);
    if (iterator == _titleToKeybindMap.end())
        return false;

    keybind = _titleToKeybindMap[keybindTitleHash];
    return true;
}

bool InputManager::IsKeyPressedInWindow(GLFWwindow* window, i32 key)
{
    return glfwGetKey(window, key) == GLFW_RELEASE ? false : true;
}

bool InputManager::IsKeyPressedByTitle(std::string keybindTitle)
{
    u32 hashedKeybindTitle = StringUtils::fnv1a_32(keybindTitle.c_str(), keybindTitle.length());
    return IsKeyPressed(hashedKeybindTitle);
}
bool InputManager::IsKeyPressed(u32 keybindTitleHash)
{
    return _titleToKeybindMap[keybindTitleHash]->state;
}