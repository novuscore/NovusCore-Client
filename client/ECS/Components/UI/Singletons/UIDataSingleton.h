#pragma once
#include <entt.hpp>
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
    };
}