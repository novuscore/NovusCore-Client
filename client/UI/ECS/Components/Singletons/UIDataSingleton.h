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
        UIDataSingleton() : destructionQueue(1000) { }

        void ClearAllElements();

        void DestroyElement(entt::entity entId, bool destroyChildren);

    public:
        robin_hood::unordered_map<entt::entity, UIScripting::BaseElement*> entityToElement;

        entt::entity focusedWidget = entt::null;
        entt::entity draggedWidget = entt::null;
        entt::entity hoveredWidget = entt::null;
        hvec2 dragOffset = hvec2(0.f,0.f);

        //Resolution
        hvec2 UIRESOLUTION = hvec2(1920.f, 1080.f);

        // Queues
        moodycamel::ConcurrentQueue<entt::entity> destructionQueue;
    };
}