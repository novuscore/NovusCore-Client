#pragma once

#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/Checkbox.h"
#include "BaseElement.h"

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
        const bool IsDraggable() const;
        const bool IsFocusable() const;
        void SetEventFlag(const UI::UITransformEventsFlags flags);
        void UnsetEventFlag(const UI::UITransformEventsFlags flags);
        void SetOnClickCallback(asIScriptFunction* callback);
        void SetOnFocusCallback(asIScriptFunction* callback);

        // Background Functions
        const std::string& GetBackgroundTexture() const;
        void SetBackgroundTexture(const std::string& texture);

        const Color GetBackgroundColor() const;
        void SetBackgroundColor(const Color& color);

        // Check Functions
        const std::string& GetCheckTexture() const;
        void SetCheckTexture(const std::string& texture);

        const Color GetCheckColor() const;
        void SetCheckColor(const Color& color);

        // Checkbox Functions
        const bool IsChecked() const;
        void SetChecked(bool checked);
        void ToggleChecked();

        void HandleKeyInput(i32 key);

        static Checkbox* CreateCheckbox();

    private:
        Panel* _checkPanel;
    };
}