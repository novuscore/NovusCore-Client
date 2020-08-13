#pragma once
#include <NovusTypes.h>
#include "../Descriptors/BufferDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct SetIndexBuffer
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            BufferID bufferID = BufferID::Invalid();
            IndexFormat indexFormat = IndexFormat::UInt32;
        };
    }
}