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

        const Color GetColor() const;
        void SetColor(const Color& color);

        static Button* CreateButton();

    private:
        Label* _label;
    };
}