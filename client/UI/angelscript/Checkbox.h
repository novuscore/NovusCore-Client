#pragma once
#include "EventElement.h"

namespace UI
{
    struct ImageStylesheet;
}

namespace UIScripting
{
    class Panel;

    class Checkbox : public EventElement
    {
    public:
        Checkbox(const std::string& name, bool collisionEnabled);

        static void RegisterType();

        // Renderable Functions
        void SetStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetFocusedStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetHoverStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetPressedStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetDisabledStylesheet(const UI::ImageStylesheet& styleSheet);

        // Check Functions
        void SetCheckStylesheet(const UI::ImageStylesheet& styleSheet);

        // Checkbox Functions
        void SetOnCheckedCallback(asIScriptFunction* callback);
        void SetOnUncheckedCallback(asIScriptFunction* callback);
        const bool IsChecked() const;
        void SetChecked(bool checked);

        void OnClick(vec2 mousePosition) override;
        bool OnKeyInput(i32 key) override;

        static Checkbox* CreateCheckbox(const std::string& name, bool collisionEnabled = true);

    private:
        Panel* _checkPanel;
    };
}