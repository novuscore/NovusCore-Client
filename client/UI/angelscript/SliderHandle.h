#pragma once
#include <NovusTypes.h>

#include "BaseElement.h"

namespace UIScripting
{
    class Slider;

    class SliderHandle : public BaseElement
    {
        friend Slider;

        SliderHandle(Slider* owningSlider);

    public:
        void OnDragged();

        static SliderHandle* CreateSliderHandle(Slider* owningSlider);

    private:
        Slider* _slider;
    };
}