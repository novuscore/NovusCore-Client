#pragma once
#include <NovusTypes.h>
#include "../Descriptors/ModelDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct DrawBindless
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 numVertices = 0;
            u32 numInstances = 0;
        };
    }
}