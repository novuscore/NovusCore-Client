#pragma once
#include <NovusTypes.h>
#include "../Descriptors/BufferDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct DispatchIndirect
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            BufferID argumentBuffer = BufferID::Invalid();
            u32 argumentBufferOffset = 0;
        };
    }
}