#pragma once
#include <NovusTypes.h>
#include "../Descriptors/GPUSemaphoreDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct AddSignalSemaphore
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            GPUSemaphoreID semaphore;
        };
    }
}