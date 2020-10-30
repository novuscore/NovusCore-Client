#pragma once
#include <NovusTypes.h>
#include "LockToken.h"

namespace UIScripting
{
    class BaseElement;
}

namespace UIUtils
{
    void RegisterNamespace();

    UIScripting::LockToken* GetLock(UIScripting::LockState state);

    UIScripting::BaseElement* GetElement(entt::entity entityId);

    vec2 GetResolution();
    f32 GetWidth();
    f32 GetHeight();
};