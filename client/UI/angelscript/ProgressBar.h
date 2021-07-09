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

    class ProgressBar : public EventElement
    {
    public:
        ProgressBar(const std::string& name, bool collisionEnabled);

        static void RegisterType();

        f32 GetCurrentValue() const;
        void SetCurrentValue(f32 current);

        // Renderable Functions
        void SetStylesheet(const UI::ImageStylesheet& styleSheet);

        static ProgressBar* CreateProgressBar(const std::string& name, bool collisionEnabled = true);

    private:
        class Panel* _panel;
    };
}