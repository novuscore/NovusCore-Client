#pragma once
#include "EventElement.h"

#include "../Stylesheets/ImageStylesheet.h"

namespace UIScripting
{
    class Panel;

    class Checkbox : public EventElement
    {
    public:
        Checkbox();

        static void RegisterType();

        // Background Functions
        void SetStylesheet(const UI::ImageStylesheet& styleSheet);

        // Check Functions
        void SetCheckStylesheet(const UI::ImageStylesheet& styleSheet);

        // Checkbox Functions
        void SetOnCheckedCallback(asIScriptFunction* callback);
        void SetOnUncheckedCallback(asIScriptFunction* callback);
        const bool IsChecked() const;
        void SetChecked(bool checked);

        void OnClick(vec2 mousePosition) override;
        bool OnKeyInput(i32 key) override;

        static Checkbox* CreateCheckbox();

    private:
        Panel* _checkPanel;
    };
}