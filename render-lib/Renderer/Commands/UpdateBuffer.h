#pragma once
#include <NovusTypes.h>

namespace Renderer
{
    namespace Commands
    {
        struct UpdateBuffer
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            BufferID dstBuffer = BufferID::Invalid();
            u64 dstBufferOffset = 0;
            u64 size = 0;
            u64 region = 0;
            void* data = 0;
        };
    }
}