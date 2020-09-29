#pragma once
#include "NovusTypes.h"
#include <entity/fwd.hpp>
#include <robin_hood.h>

namespace UIScripting
{
    class BaseElement;
}

namespace std
{
    class shared_mutex;
}

namespace UISingleton
{
    struct UIDataSingleton
    {
    public:
        UIDataSingleton() { }

        void ClearAllElements();
    public:
        robin_hood::unordered_map<entt::entity, UIScripting::BaseElement*> entityToElement;

        entt::entity focusedWidget = entt::null;
        entt::entity draggedWidget = entt::null;
        entt::entity hoveredWidget = entt::null;
        hvec2 dragOffset = hvec2(0.f,0.f);

        //Resolution
        hvec2 UIRESOLUTION = hvec2(1920.f, 1080.f);
    };
}