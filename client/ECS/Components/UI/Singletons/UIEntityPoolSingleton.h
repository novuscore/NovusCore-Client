#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <entt.hpp>

namespace UI
{
    struct UIEntityPoolSingleton
    {
        const u32 ENTITIES_TO_PREALLOCATE = 3;

    public:
        UIEntityPoolSingleton() : entityIdPool(ENTITIES_TO_PREALLOCATE) { }

        entt::entity DeQeueueId();

        void AllocatePool();

    private:
        moodycamel::ConcurrentQueue<entt::entity> entityIdPool;
    };
}