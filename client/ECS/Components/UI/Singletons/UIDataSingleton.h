#pragma once
#include "NovusTypes.h"
#include <entity/fwd.hpp>
#include <robin_hood.h>

namespace UI
{
    class asUITransform;

    struct UIDataSingleton
    {
    public:
        UIDataSingleton() : entityToAsObject(), focusedWidget(entt::null) { }

        void ClearWidgets();

        void DestroyWidget(entt::entity entId);

    public:
        robin_hood::unordered_map<entt::entity, UI::asUITransform*> entityToAsObject;

        entt::entity focusedWidget;

        //Resolution
        vec2 UIRESOLUTION = vec2(1920, 1080);
    };
}