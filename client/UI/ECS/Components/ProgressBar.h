#pragma once
#include <NovusTypes.h>

class asIScriptFunction;

namespace UI
{
    enum class ProgressBarFillStyle
    {
        LEFT,
        RIGHT,
        CENTER
    };
}

namespace UIComponent
{
    struct ProgressBar
    {
    public:
        ProgressBar() { }

        f32 currentValue = 0.f;
        UI::ProgressBarFillStyle fillStyle = UI::ProgressBarFillStyle::LEFT;
    };
}