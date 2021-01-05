#pragma once
#include <NovusTypes.h>
#include "BaseElement.h"

#include "../Stylesheets/ImageStylesheet.h"
#include "../Stylesheets/TextStylesheet.h"

namespace UIScripting
{
    class Label;

    class Button : public BaseElement
    {
    public:
        Button();

        static void RegisterType();

        //Button Functions.
        const bool IsClickable() const;
        void SetOnClickCallback(asIScriptFunction* callback);

        //Label Functions
        const std::string GetText() const;
        void SetText(const std::string& text);

        void SetTextStylesheet(const UI::TextStylesheet& textStylesheet);

        //Panel Functions
        void SetStylesheet(UI::ImageStylesheet stylesheet);

        static Button* CreateButton();

    private:
        Label* _label;
    };
}