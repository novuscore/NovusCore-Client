#pragma once
#include <Utils/ConcurrentQueue.h>
#include <entity/fwd.hpp>

namespace UISingleton
{
    struct UIEntityPoolSingleton
    {
        const int ENTITIES_TO_PREALLOCATE = 10000;

    public:
        UIEntityPoolSingleton() : entityIdPool(ENTITIES_TO_PREALLOCATE) { }

        void AllocatePool();
        entt::entity GetId();
    private:
        moodycamel::ConcurrentQueue<entt::entity> entityIdPool;
    };
}