#pragma once
#include <NovusTypes.h>
#include "../Descriptors/BufferDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct DrawIndexedIndirectCount
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            BufferID argumentBuffer = BufferID::Invalid();
            BufferID drawCountBuffer = BufferID::Invalid();
            u32 argumentBufferOffset = 0;
            u32 drawCountBufferOffset = 0;
            u32 maxDrawCount = 0;
        };
    }
}