#pragma once
#include "BaseElement.h"

#include "../ECS/Components/Image.h"

namespace UIScripting
{
    class Panel;

    class Checkbox : public BaseElement
    {
    public:
        Checkbox();

        static void RegisterType();

        // TransformEvents Functions
        const bool IsClickable() const;
        const bool IsFocusable() const;
        void SetOnClickCallback(asIScriptFunction* callback);
        void SetOnFocusGainedCallback(asIScriptFunction* callback);
        void SetOnFocusLostCallback(asIScriptFunction* callback);

        // Background Functions
        void SetStylesheet(UI::ImageStylesheet styleSheet);

        // Check Functions
        void SetCheckStylesheet(UI::ImageStylesheet styleSheet);

        // Checkbox Functions
        void SetOnCheckedCallback(asIScriptFunction* callback);
        void SetOnUncheckedCallback(asIScriptFunction* callback);
        const bool IsChecked() const;
        void SetChecked(bool checked);
        void ToggleChecked();

        void HandleKeyInput(i32 key);

        static Checkbox* CreateCheckbox();

    private:
        Panel* _checkPanel;
    };
}