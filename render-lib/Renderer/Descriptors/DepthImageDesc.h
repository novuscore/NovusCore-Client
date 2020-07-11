#pragma once
#include <NovusTypes.h>
#include <Utils/StrongTypedef.h>
#include "../RenderStates.h"

namespace Renderer
{
    struct DepthImageDesc
    {
        std::string debugName = "";

        vec2 dimensions = vec2(0, 0);
        ImageDimensionType dimensionType = ImageDimensionType::DIMENSION_ABSOLUTE;

        DepthImageFormat format = DEPTH_IMAGE_FORMAT_UNKNOWN;
        SampleCount sampleCount = SAMPLE_COUNT_1;
        f32 depthClearValue = 1.0f;
        u8 stencilClearValue = 0;
    };

    // Lets strong-typedef an ID type with the underlying type of u16
    STRONG_TYPEDEF(DepthImageID, u16);
}