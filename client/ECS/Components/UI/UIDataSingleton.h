#pragma once
#include <entt.hpp>

struct UIDataSingleton
{
public:
    UIDataSingleton() : focusedWidget() { }

    entt::entity focusedWidget;
};