#pragma once
#include <NovusTypes.h>

namespace Renderer
{
    namespace Commands
    {
        struct PushConstant
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            void* data = nullptr;
            u32 offset = 0;
            u32 size = 0;
        };
    }
}