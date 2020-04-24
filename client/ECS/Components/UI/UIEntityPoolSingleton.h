#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <entt.hpp>

struct UIEntityPoolSingleton
{
public:
    UIEntityPoolSingleton() : entityIdPool(10000) { }

    moodycamel::ConcurrentQueue<entt::entity> entityIdPool;
};