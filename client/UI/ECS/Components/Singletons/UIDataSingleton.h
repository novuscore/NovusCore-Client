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
        UIDataSingleton() : entityToAsObject(), focusedWidget(entt::null) { }

        void ClearWidgets();

        void DestroyWidget(entt::entity entId);

    public:
        robin_hood::unordered_map<entt::entity, UIScripting::BaseElement*> entityToAsObject;

        entt::entity focusedWidget;

        //Resolution
        vec2 UIRESOLUTION = vec2(1920, 1080);
    };
}