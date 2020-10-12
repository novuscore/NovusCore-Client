#pragma once
#include <NovusTypes.h>

namespace Renderer
{
    namespace Commands
    {
        struct FillBuffer
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            BufferID dstBuffer = BufferID::Invalid();
            u64 dstBufferOffset = 0;
            u64 size = 0;
            u64 region = 0;
            u32 data = 0;
        };
    }
}