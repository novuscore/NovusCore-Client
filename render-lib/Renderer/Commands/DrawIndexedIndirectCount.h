#pragma once
#include <NovusTypes.h>

namespace Renderer
{
    namespace Commands
    {
        struct DrawIndexedIndirectCount
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            void* argumentBuffer = nullptr;
            void* drawCountBuffer = nullptr;
            u32 argumentBufferOffset = 0;
            u32 drawCountBufferOffset = 0;
            u32 maxDrawCount = 0;
        };
    }
}