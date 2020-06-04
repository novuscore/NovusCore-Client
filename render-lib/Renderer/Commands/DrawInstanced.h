#pragma once
#include <NovusTypes.h>
#include "../Descriptors/ModelDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct DrawInstanced
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            ModelID model = ModelID::Invalid();
            u32 count = 0;
        };
    }
}