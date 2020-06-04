#pragma once
#include <NovusTypes.h>
#include "../Descriptors/SamplerDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct SetSampler
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 slot = 0;
            SamplerID sampler = SamplerID::Invalid();
        };
    }
}