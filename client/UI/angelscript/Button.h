#pragma once
#include <NovusTypes.h>
#include "EventElement.h"

#include "../Stylesheets/ImageStylesheet.h"
#include "../Stylesheets/TextStylesheet.h"

namespace UIScripting
{
    class Label;

    class Button : public EventElement
    {
    public:
        Button(const std::string& name, bool collisionEnabled);

        static void RegisterType();

        //Label Functions
        const std::string GetText() const;
        void SetText(const std::string& text);

        void SetTextStylesheet(const UI::TextStylesheet& textStylesheet);

        //Panel Functions
        void SetStylesheet(const UI::ImageStylesheet& stylesheet);
        void SetFocusedStylesheet(const UI::ImageStylesheet& stylesheet);
        void SetHoveredStylesheet(const UI::ImageStylesheet& stylesheet);
        void SetPressedStylesheet(const UI::ImageStylesheet& stylesheet);
        void SetDisabledStylesheet(const UI::ImageStylesheet& stylesheet);

        static Button* CreateButton(const std::string& name, bool collisionEnabled = true);

    private:
        Label* _label;
    };
}