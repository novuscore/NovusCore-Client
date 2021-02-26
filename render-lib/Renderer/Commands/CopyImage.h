#pragma once
#include <NovusTypes.h>

#include "../Descriptors/ImageDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct CopyImage
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            ImageID dstImage = ImageID::Invalid();
            uvec2 dstPos = uvec2(0, 0);
            u32 dstMipLevel = 0;
            ImageID srcImage = ImageID::Invalid();
            uvec2 srcPos = uvec2(0, 0);
            u32 srcMipLevel = 0;
            uvec2 size = uvec2(0, 0);
        };
    }
}