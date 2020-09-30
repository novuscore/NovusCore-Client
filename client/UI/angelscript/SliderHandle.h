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

        SliderHandle(Slider* owningSlider);

    public:
        void OnDragged();

        static SliderHandle* CreateSliderHandle(Slider* owningSlider);

    private:
        Slider* _slider;
    };
}