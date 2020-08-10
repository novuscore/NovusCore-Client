#pragma once
#include <NovusTypes.h>
#include <Renderer/Descriptors/BufferDesc.h>

namespace Renderer
{
    namespace Commands
    {
        struct PipelineBarrier
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            PipelineBarrierType barrierType;
            BufferID buffer = BufferID::Invalid();
        };
    }
}