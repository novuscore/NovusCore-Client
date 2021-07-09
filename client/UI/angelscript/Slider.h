#pragma once
#include <NovusTypes.h>
#include "EventElement.h"

namespace UI
{
    struct ImageStylesheet;
}

namespace UIScripting
{
    class SliderHandle;

    class Slider : public EventElement
    {
    public:
        Slider(const std::string& name, bool collisionEnabled);

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

        void SetOnValueChangedCallback(asIScriptFunction* callback);

        // Renderable Functions
        void SetStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetFocusedStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetHoverStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetPressedStylesheet(const UI::ImageStylesheet& styleSheet);

        // Handle Functions
        void SetHandleStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetHandleFocusedStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetHandleHoverStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetHandlePressedStylesheet(const UI::ImageStylesheet& styleSheet);

        void OnClick(vec2 mousePosition) override;
        bool OnKeyInput(i32 key) override;

        void SetHandleSize(const vec2& size);
        void UpdateHandlePosition();

        static Slider* CreateSlider(const std::string& name, bool collisionEnabled = true);

    private:
        SliderHandle* _handle = nullptr;
    };
}