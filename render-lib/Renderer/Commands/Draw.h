#pragma once
#include <NovusTypes.h>
#include "../Descriptors/ModelDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct Draw
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            ModelID model = ModelID::Invalid();
        };
    }
}