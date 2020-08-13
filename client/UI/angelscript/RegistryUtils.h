#pragma once
#include <NovusTypes.h>
#include "LockToken.h"

namespace UIUtils::Registry
{
    void RegisterNamespace();

    UIScripting::LockToken* GetLock(UIScripting::LockState state);
};