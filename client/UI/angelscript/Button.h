#pragma once
#include <NovusTypes.h>
#include "BaseElement.h"

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

        const Color& GetTextColor() const;
        void SetTextColor(const Color& color);

        const Color& GetTextOutlineColor() const;
        void SetTextOutlineColor(const Color& outlineColor);

        const f32 GetTextOutlineWidth() const;
        void SetTextOutlineWidth(f32 outlineWidth);

        void SetFont(std::string fontPath, f32 fontSize);

        //Panel Functions        
        const std::string& GetTexture() const;
        void SetTexture(const std::string& texture);

        void SetTexCoord(const vec4& texCoords);

        const Color GetColor() const;
        void SetColor(const Color& color);

        const std::string& GetBorder() const;
        void SetBorder(const std::string& texture);

        void SetBorderSize(const u32 topSize, const u32 rightSize, const u32 bottomSize, const u32 leftSize);
        void SetBorderInset(const u32 topBorderInset, const u32 rightBorderInset, const u32 bottomBorderInset, const u32 leftBorderInset);

        void SetSlicing(const u32 topOffset, const u32 rightOffset, const u32 bottomOffset, const u32 leftOffset);

        static Button* CreateButton();

    private:
        Label* _label;
    };
}