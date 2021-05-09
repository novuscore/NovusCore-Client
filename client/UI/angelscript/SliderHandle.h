#pragma once
#include <NovusTypes.h>

#include "EventElement.h"

namespace UIScripting
{
    class Slider;

    class SliderHandle : public EventElement
    {
        friend Slider;

        SliderHandle(Slider* owningSlider, const std::string& name, bool collisionEnabled = true);

    public:
        void OnDrag() override;

    private:
        Slider* _slider;
    };
}