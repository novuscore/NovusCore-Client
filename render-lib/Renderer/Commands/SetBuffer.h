#pragma once
#include <NovusTypes.h>
#include "../Descriptors/BufferDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct SetBuffer
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 slot = 0;
            BufferID buffer = BufferID::Invalid();
        };
    }
}