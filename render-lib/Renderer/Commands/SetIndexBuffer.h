#pragma once
#include <NovusTypes.h>
#include "../Descriptors/ModelDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct SetIndexBuffer
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            ModelID modelID = ModelID::Invalid();
        };
    }
}