#pragma once
#include "NovusTypes.h"
#include <entity/fwd.hpp>
#include <robin_hood.h>

namespace UIScripting
{
    class BaseElement;
}

namespace UISingleton
{
    struct UIDataSingleton
    {
    public:
        UIDataSingleton() { }

        robin_hood::unordered_map<entt::entity, UIScripting::BaseElement*> entityToElement;

        entt::entity focusedWidget = entt::null;
        entt::entity draggedWidget = entt::null;
        entt::entity hoveredWidget = entt::null;
        hvec2 dragOffset = hvec2(0.f,0.f);

        //Resolution
        const f32 referenceHeight = 1080.f;
        hvec2 UIRESOLUTION = hvec2(0.0f, 0.f);
    };
}