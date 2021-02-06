#pragma once
#include <NovusTypes.h>
#include <Utils/StrongTypedef.h>
#include "../RenderStates.h"

namespace Renderer
{
    struct ImageDesc
    {
        std::string debugName = "";

        vec2 dimensions = vec2(0, 0);
        ImageDimensionType dimensionType = ImageDimensionType::DIMENSION_ABSOLUTE;

        u32 depth = 1;
        u32 mipLevels = 1;
        ImageFormat format = ImageFormat::UNKNOWN;
        SampleCount sampleCount = SampleCount::SAMPLE_COUNT_1;
        Color clearColor = Color::Clear;
    };

    // Lets strong-typedef an ID type with the underlying type of u16
    STRONG_TYPEDEF(ImageID, u16);
}