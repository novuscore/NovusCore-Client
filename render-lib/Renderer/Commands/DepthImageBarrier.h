#pragma once
#include <NovusTypes.h>
#include "../BackendDispatch.h"
#include "../Descriptors/DepthImageDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct DepthImageBarrier
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            DepthImageID image = DepthImageID::Invalid();
        };
    }
}