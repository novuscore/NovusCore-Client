#pragma once
#include <NovusTypes.h>
#include "../Descriptors/BufferDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct DrawIndexed
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 indexCount = 0;
            u32 instanceCount = 0;
            u32 indexOffset = 0;
            u32 vertexOffset = 0;
            u32 instanceOffset = 0;
        };
    }
}