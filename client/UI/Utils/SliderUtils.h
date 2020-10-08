#pragma once
#include <NovusTypes.h>
#include "../ECS/Components/Slider.h"

namespace UIUtils::Slider
{
    inline static f32 GetPercent(const UIComponent::Slider* slider)
    {
        return (slider->currentValue - slider->minimumValue) / (slider->maximumValue - slider->minimumValue);
    }

    inline static f32 GetValueFromPercent(const UIComponent::Slider* slider, f32 percent)
    {
        return (Math::Clamp(percent, 0.f, 1.f) * (slider->maximumValue - slider->minimumValue)) + slider->minimumValue;
    }
};