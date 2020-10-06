#pragma once
#include <NovusTypes.h>

namespace UIComponent
{
    struct Slider
    {
    public:
        Slider() { }

        f32 minimumValue = 0.f;
        f32 maximumValue = 100.f;
        f32 currentValue = 50.f;
    };
}