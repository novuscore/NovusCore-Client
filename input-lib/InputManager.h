#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>
#include <memory>
#include "Keybind.h"

class Window;
struct GLFWwindow;

typedef void MousePositionUpdateFunc(Window* window, f32 x, f32 y);
typedef void MouseScrollUpdateFunc(Window* window, f32 x, f32 y);
typedef bool KeyboardInputCallbackFunc(Window* window, i32 key, i32 actionMask, i32 modifierMask);
typedef bool CharInputCallbackFunc(Window* window, u32 unicodeKey);

class InputManager
{
public:
    InputManager();
    void KeyboardInputHandler(Window* window, i32 key, i32 scancode, i32 actionMask, i32 modifierMask);
    void CharInputHandler(Window* window, u32 unicodeKey);
    void MouseInputHandler(Window* window, i32 button, i32 actionMask, i32 modifierMask);
    void MousePositionHandler(Window* window, f32 x, f32 y);
    void MouseScrollHandler(Window* window, f32 x, f32 y);

    bool RegisterKeybind(std::string keybindTitle, i32 key, i32 actionMask, i32 modifierMask, std::function<KeybindCallbackFunc> callback = nullptr);
    bool UnregisterKeybind(std::string keybindTitle);

    bool RegisterKeyboardInputCallback(u32 callbackNameHash, std::function<KeyboardInputCallbackFunc> callback);
    bool UnregisterKeyboardInputCallback(u32 callbackNameHash);

    bool RegisterCharInputCallback(u32 callbackNameHash, std::function<CharInputCallbackFunc> callback);
    bool UnregisterCharInputCallback(u32 callbackNameHash);

    bool RegisterMousePositionCallback(std::string callbackName, std::function<MousePositionUpdateFunc> callback);
    bool UnregisterMousePositionCallback(std::string callbackName);

    bool RegisterMouseScrollCallback(std::string callbackName, std::function<MouseScrollUpdateFunc> callback);
    bool UnregisterMouseScrollCallback(std::string callbackName);

    bool GetKeybind(std::string keybindTitle, std::shared_ptr<Keybind>& keybind);
    bool IsKeyPressedInWindow(GLFWwindow* window, i32 key);
    bool IsKeyPressedByTitle(std::string keybindTitle);
    bool IsKeyPressed(u32 keybindTitleHash);

    hvec2 GetMousePosition() { return hvec2(_mousePositionX, _mousePositionY); }
    f32 GetMousePositionX() { return _mousePositionX; }
    f32 GetMousePositionY() { return _mousePositionY; }
    bool IsMousePressed() { return _mouseState; }
private:
    robin_hood::unordered_map<i32, robin_hood::unordered_map<u32, std::shared_ptr<Keybind>>> _keyToKeybindMap;
    robin_hood::unordered_map<u32, std::shared_ptr<Keybind>> _titleToKeybindMap;
    robin_hood::unordered_map<u32, std::function<KeyboardInputCallbackFunc>> _keyboardInputCallbackMap;
    robin_hood::unordered_map<u32, std::function<CharInputCallbackFunc>> _charInputCallbackMap;
    robin_hood::unordered_map<u32, std::function<MousePositionUpdateFunc>> _mousePositionUpdateCallbacks;
    robin_hood::unordered_map<u32, std::function<MouseScrollUpdateFunc>> _mouseScrollUpdateCallbacks;
    f32 _mousePositionX = 0;
    f32 _mousePositionY = 0;
    bool _mouseState = false;
};