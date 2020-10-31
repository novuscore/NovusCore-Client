#pragma once
#include <NovusTypes.h>
#include <functional>
#include <Utils/StringUtils.h>
#include <Memory>

enum KeybindAction
{
    KEYBIND_ACTION_RELEASE = 0x01 << 0,
    KEYBIND_ACTION_PRESS = 0x01 << 1,
    KEYBIND_ACTION_REPEAT = 0x01 << 2,

    // Combinations
    KEYBIND_ACTION_CLICK = KEYBIND_ACTION_RELEASE | KEYBIND_ACTION_PRESS,
    KEYBIND_ACTION_ANY = KEYBIND_ACTION_RELEASE | KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT
};
enum KeybindModifier
{
    KEYBIND_MOD_NONE = 0x01 << 0,
    KEYBIND_MOD_SHIFT = 0x01 << 1,
    KEYBIND_MOD_CONTROL = 0x01 << 2,
    KEYBIND_MOD_ALT = 0x01 << 3,

    // Combinations
    KEYBIND_MOD_ANY = KEYBIND_MOD_NONE | KEYBIND_MOD_SHIFT | KEYBIND_MOD_CONTROL | KEYBIND_MOD_ALT
};

class Window;
class Keybind;
typedef bool KeybindCallbackFunc(Window*, std::shared_ptr<Keybind>);
class Keybind
{
public:
    Keybind() : title("invalid"), hashedName(StringUtils::fnv1a_32(title.c_str(), title.length())), actionMask(0), key(-1), modifierMask(0), state(0), callback(nullptr) { }
    Keybind(std::string inTitle, i32 inActionMask, i32 inKey, i32 inModifierMask, std::function<KeybindCallbackFunc> inCallback) : title(inTitle), hashedName(StringUtils::fnv1a_32(title.c_str(), title.length())), actionMask(inActionMask), key(inKey), modifierMask(inModifierMask), state(0), callback(inCallback) { }

public:
    std::string title;
    u32 hashedName;
    i32 actionMask;
    i32 key;
    i32 modifierMask;

    // Runtime Values
    i32 state;
    i32 currentModifierMask;

    std::function<KeybindCallbackFunc> callback;
};