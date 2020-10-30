#pragma once

#include "BaseElement.h"
#include "../ECS/Components/TransformEvents.h"

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
        void SetEventFlag(const UI::TransformEventsFlags flags);
        void UnsetEventFlag(const UI::TransformEventsFlags flags);
        void SetOnClickCallback(asIScriptFunction* callback);
        void SetOnFocusCallback(asIScriptFunction* callback);

        // Background Functions
        const std::string& GetTexture() const;
        void SetTexture(const std::string& texture);
        const Color GetColor() const;
        void SetColor(const Color& color);

        const std::string& GetBorder() const;
        void SetBorder(const std::string& texture);
        void SetBorderSize(const u32 topSize, const u32 rightSize, const u32 bottomSize, const u32 leftSize);
        void SetBorderInset(const u32 topBorderInset, const u32 rightBorderInset, const u32 bottomBorderInset, const u32 leftBorderInset);
        void SetSlicing(const u32 topOffset, const u32 rightOffset, const u32 bottomOffset, const u32 leftOffset);

        // Check Functions
        const std::string& GetCheckTexture() const;
        void SetCheckTexture(const std::string& texture);
        const Color GetCheckColor() const;
        void SetCheckColor(const Color& color);

        const std::string& GetCheckBorder() const;
        void SetCheckBorder(const std::string& texture);
        void SetCheckBorderSize(const u32 topSize, const u32 rightSize, const u32 bottomSize, const u32 leftSize);
        void SetCheckBorderInset(const u32 topBorderInset, const u32 rightBorderInset, const u32 bottomBorderInset, const u32 leftBorderInset);
        void SetCheckSlicing(const u32 topOffset, const u32 rightOffset, const u32 bottomOffset, const u32 leftOffset);

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