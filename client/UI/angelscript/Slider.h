#pragma once
#include <NovusTypes.h>

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

        void SetHandleSize(const vec2& size);

        void OnClicked(hvec2 mousePosition);

        static Slider* CreateSlider();

    private:
        SliderHandle* _handle = nullptr;
    };
}