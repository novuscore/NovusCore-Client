#pragma once
#include <shared_mutex>

namespace UISingleton
{
    struct UILockSingleton
    {
    public:
        UILockSingleton() { }

        std::shared_mutex mutex;
    };
}