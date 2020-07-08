#pragma once
#include <Utils/ConcurrentQueue.h>
#include <entity/fwd.hpp>

namespace UI
{
    struct UIEntityPoolSingleton
    {
        const int ENTITIES_TO_PREALLOCATE = 10000;

    public:
        UIEntityPoolSingleton() : entityIdPool(ENTITIES_TO_PREALLOCATE) { }

        entt::entity DeQeueueId();

        void AllocatePool();

    private:
        moodycamel::ConcurrentQueue<entt::entity> entityIdPool;
    };
}