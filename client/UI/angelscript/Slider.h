#pragma once
#include <NovusTypes.h>

#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Image.h"
#include "BaseElement.h"

namespace UIScripting
{
    class SliderHandle;

    class Slider : public BaseElement
    {
    public:
        Slider();

        static void RegisterType();

        float GetMinValue();
        void SetMinValue(float min);
        float GetMaxValue();
        void SetMaxValue(float max);
        float GetCurrentValue();
        void SetCurrentValue(float current);

        const std::string& GetTexture() const;
        void SetTexture(const std::string& texture);

        const Color GetColor() const;
        void SetColor(const Color& color);

        // Handle functions.
        const std::string& GetHandleTexture() const;
        void SetHandleTexture(const std::string& texture);

        const Color GetHandleColor() const;
        void SetHandleColor(const Color& color);

        static Slider* CreateSlider();

    private:
        SliderHandle* sliderHandle = nullptr;
    };
}