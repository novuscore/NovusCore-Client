#pragma once
#include <NovusTypes.h>
#include "../Descriptors/ModelDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct Dispatch
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 threadGroupCountX = 0;
            u32 threadGroupCountY = 0;
            u32 threadGroupCountZ = 0;
        };
    }
}