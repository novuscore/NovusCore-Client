#pragma once
#include "NovusTypes.h"
#include <entity/fwd.hpp>
#include <robin_hood.h>
#include <Utils/ConcurrentQueue.h>

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
        UIDataSingleton() : entityToAsObject(), focusedWidget(entt::null), destructionQueue(1000), visibilityToggleQueue(1000), collisionToggleQueue(1000) { }

        std::shared_mutex& GetMutex(entt::entity entId);

        void ClearWidgets();

        void DestroyWidget(entt::entity entId);

    public:
        robin_hood::unordered_map<entt::entity, UIScripting::BaseElement*> entityToAsObject;

        entt::entity focusedWidget;

        //Resolution
        vec2 UIRESOLUTION = vec2(1920, 1080);

        moodycamel::ConcurrentQueue<entt::entity> destructionQueue;
        moodycamel::ConcurrentQueue<entt::entity> visibilityToggleQueue;
        moodycamel::ConcurrentQueue<entt::entity> collisionToggleQueue;
    };
}