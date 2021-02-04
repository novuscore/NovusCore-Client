#pragma once
#include <NovusTypes.h>
#include "../Descriptors/ImageDesc.h"
#include "../Descriptors/DepthImageDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct ClearImage
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            ImageID image = ImageID::Invalid();
            Color color = Color::Clear;
        };
        
        struct ClearDepthImage
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            DepthImageID image = DepthImageID::Invalid();
            DepthClearFlags flags = DepthClearFlags::BOTH;
            f32 depth = 0.0f;
            u8 stencil = 0;
        };
    }
}