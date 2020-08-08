#pragma once
#include <NovusTypes.h>
#include "../Descriptors/GPUSemaphoreDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct CopyBuffer
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            BufferID dstBuffer = BufferID::Invalid();
            u64 dstBufferOffset = 0;
            BufferID srcBuffer = BufferID::Invalid();
            u64 srcBufferOffset = 0;
            u64 region = 0;
        };
    }
}