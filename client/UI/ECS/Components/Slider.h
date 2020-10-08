#pragma once
#include <NovusTypes.h>

class asIScriptFunction;

namespace UIComponent
{
    struct Slider
    {
    public:
        Slider() { }

        asIScriptFunction* onValueChanged = nullptr;

        f32 minimumValue = 0.f;
        f32 maximumValue = 100.f;
        f32 currentValue = 50.f;

        f32 stepSize = 0.f;
    };
}