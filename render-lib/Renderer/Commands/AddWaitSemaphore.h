#pragma once
#include <NovusTypes.h>
#include "../Descriptors/SemaphoreDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct AddWaitSemaphore
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            SemaphoreID semaphore;
        };
    }
}