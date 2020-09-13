#pragma once
#include <NovusTypes.h>

#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Image.h"
#include "BaseElement.h"

namespace UIScripting
{
    class Slider;

    class SliderHandle : public BaseElement
    {
        friend Slider;

    private:
        SliderHandle(Slider* owningSlider);

        static SliderHandle* CreateSliderHandle(Slider* owningSlider);

        Slider* _slider;
    };
}