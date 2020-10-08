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

        f32 GetMinValue() const;
        void SetMinValue(f32 min);
        f32 GetMaxValue() const;
        void SetMaxValue(f32 max);
        f32 GetCurrentValue() const;
        void SetCurrentValue(f32 current);
        f32 GetPercentValue() const;
        void SetPercentValue(f32 value);

        f32 GetStepSize() const;
        void SetStepSize(f32 stepSize);

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

        void SetOnValueChangedCallback(asIScriptFunction* callback);

        void UpdateHandlePosition();

        static Slider* CreateSlider();

    private:
        SliderHandle* _handle = nullptr;
    };
}