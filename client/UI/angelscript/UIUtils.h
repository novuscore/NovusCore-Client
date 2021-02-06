#pragma once
#include <NovusTypes.h>

namespace UIScripting
{
    class BaseElement;
}

namespace UIUtils
{
    void RegisterNamespace();

    UIScripting::BaseElement* GetElement(entt::entity entityId);

    vec2 GetResolution();
};