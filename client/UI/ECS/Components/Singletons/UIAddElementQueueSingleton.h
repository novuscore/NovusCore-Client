#pragma once
#include <Utils/ConcurrentQueue.h>
#include <entity/fwd.hpp>
#include "../../../../UI/UITypes.h"

namespace UI
{
    struct UIElementCreationData
    {
        entt::entity entityId;
        UIElementType type;
        void* asObject;
    };
}

namespace UISingleton
{
    struct UIAddElementQueueSingleton
    {
    public:
        UIAddElementQueueSingleton() : elementPool(1024) { }

        moodycamel::ConcurrentQueue<UI::UIElementCreationData> elementPool;
    };
}