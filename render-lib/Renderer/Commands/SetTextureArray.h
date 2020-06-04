#pragma once
#include <NovusTypes.h>
#include "../Descriptors/TextureArrayDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct SetTextureArray
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            u32 slot = 0;
            TextureArrayID textureArray = TextureArrayID::Invalid();
        };
    }
}