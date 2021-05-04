#pragma once
#include <NovusTypes.h>
#include "EventElement.h"

#include "../Stylesheets/ImageStylesheet.h"

namespace UIScripting
{
    class Panel : public EventElement
    {
    public:
        Panel(bool collisionEnabled = true);

        static void RegisterType();

        // Renderable Functions
        void SetStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetFocusedStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetHoverStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetPressedStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetDisabledStylesheet(const UI::ImageStylesheet& styleSheet);

        static Panel* CreatePanel(bool collisionEnabled = true);
    };
}