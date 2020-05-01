#pragma once
#include <entt.hpp>
#include <robin_hood.h>

namespace UI
{
    class asUITransform;

    struct UIDataSingleton
    {
    public:
        UIDataSingleton() : focusedWidget(), entityToAsObject() { }

        entt::entity focusedWidget;

        robin_hood::unordered_map<entt::entity, UI::asUITransform*> entityToAsObject;
    };
}