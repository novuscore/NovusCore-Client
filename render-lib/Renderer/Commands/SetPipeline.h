#pragma once
#include <NovusTypes.h>
#include "../Descriptors/GraphicsPipelineDesc.h"
#include "../Descriptors/ComputePipelineDesc.h"

namespace Renderer
{
    namespace Commands
    {
        struct BeginGraphicsPipeline
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            GraphicsPipelineID pipeline = GraphicsPipelineID::Invalid();
        };

        struct EndGraphicsPipeline
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            GraphicsPipelineID pipeline = GraphicsPipelineID::Invalid();
        };
        
        struct BeginComputePipeline
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            ComputePipelineID pipeline = ComputePipelineID::Invalid();
        };

        struct EndComputePipeline
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            ComputePipelineID pipeline = ComputePipelineID::Invalid();
        };
    }
}