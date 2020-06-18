#pragma once
#include <NovusTypes.h>
#include "../Descriptors/ModelDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct SetBuffer
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 slot = 0;
            void* buffer = nullptr;
        };
    }
}