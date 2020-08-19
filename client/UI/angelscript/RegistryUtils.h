#pragma once
#include <NovusTypes.h>
#include "LockToken.h"

namespace UIScripting
{
    class BaseElement;
}

namespace UIUtils::Registry
{
    void RegisterNamespace();

    UIScripting::LockToken* GetLock(UIScripting::LockState state);

    UIScripting::BaseElement* GetElement(entt::entity entityId);
};