#pragma once
#include <NovusTypes.h>
#include "../Descriptors/ModelDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct DrawIndexedBindless
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            ModelID modelID = ModelID::Invalid();
            u32 numVertices = 0;
            u32 numInstances = 0;
        };
    }
}