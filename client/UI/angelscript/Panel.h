#pragma once
#include <NovusTypes.h>
#include "EventElement.h"

namespace UI
{
    struct ImageStylesheet;
}

namespace UIScripting
{
    class Panel : public EventElement
    {
    public:
        Panel(const std::string& name, bool collisionEnabled);

        static void RegisterType();

        // Renderable Functions
        void SetStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetFocusedStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetHoverStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetPressedStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetDisabledStylesheet(const UI::ImageStylesheet& styleSheet);

        static Panel* CreatePanel(const std::string& name, bool collisionEnabled = true);
    };
}