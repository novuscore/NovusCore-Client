#pragma once
#include <NovusTypes.h>
#include "../Descriptors/TextureDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct SetTexture
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 slot = 0;
            TextureID texture = TextureID::Invalid();
        };
    }
}