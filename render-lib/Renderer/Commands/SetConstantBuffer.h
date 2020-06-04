#pragma once
#include <NovusTypes.h>
#include "../ConstantBuffer.h"

namespace Renderer
{
    namespace Commands
    {
        struct SetConstantBuffer
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 slot = 0;
            void* gpuResource = nullptr;
            size_t frameIndex = 0;
        };
    }
}