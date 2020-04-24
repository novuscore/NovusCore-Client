#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <entt.hpp>

struct UIElementData
{
    enum class UIElementType
    {
        UITYPE_PANEL,
        UITYPE_TEXT,
        UITYPE_BUTTON,
        UITYPE_INPUT
    };

    entt::entity entityId;
    UIElementType type;
};

struct UIAddElementQueueSingleton
{
public:
    UIAddElementQueueSingleton() : elementPool(1024) { }

    moodycamel::ConcurrentQueue<UIElementData> elementPool;
};