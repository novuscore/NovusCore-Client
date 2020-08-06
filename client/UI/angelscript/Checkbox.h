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
        void SetEventFlag(const UI::UITransformEventsFlags flags);
        void UnsetEventFlag(const UI::UITransformEventsFlags flags);
        const bool IsClickable() const { return _events->IsClickable(); }
        const bool IsDraggable() const { return _events->IsDraggable(); }
        const bool IsFocusable() const { return _events->IsFocusable(); }
        void SetOnClickCallback(asIScriptFunction* callback);
        void SetOnDragCallback(asIScriptFunction* callback);
        void SetOnFocusCallback(asIScriptFunction* callback);

        // Background Functions
        void SetBackgroundTexture(const std::string& texture);
        const std::string& GetBackgroundTexture() const { return _image->texture; }

        void SetBackgroundColor(const Color& color);
        const Color GetBackgroundColor() const { return _image->color; }

        // Check Functions
        void SetCheckTexture(const std::string& texture);
        const std::string& GetCheckTexture() const;

        void SetCheckColor(const Color& color);
        const Color GetCheckColor() const;

        // Checkbox Functions
        void ToggleChecked();
        void SetChecked(bool checked);
        bool IsChecked() { return _checkBox->checked; }

        void HandleKeyInput(i32 key);

        static Checkbox* CreateCheckbox();

    private:
        UIComponent::TransformEvents* _events;
        UIComponent::Checkbox* _checkBox;
        UIComponent::Image* _image;

        Panel* checkPanel;
    };
}