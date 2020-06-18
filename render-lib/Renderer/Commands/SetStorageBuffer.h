#pragma once
#include <NovusTypes.h>

namespace Renderer
{
    namespace Commands
    {
        struct SetStorageBuffer
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 slot = 0;
            void* descriptor = nullptr;
            size_t frameIndex = 0;
        };
    }
}