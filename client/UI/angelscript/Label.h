#pragma once
#include <NovusTypes.h>
#include "../UITypes.h"
#include "BaseElement.h"

namespace UIScripting
{
    class Label : public BaseElement
    {
    public:
        Label();

        static void RegisterType();

        //Text Functions
        const std::string GetText() const;
        void SetText(const std::string& newText);

        void SetFont(const std::string& fontPath, f32 fontSize);

        const Color& GetColor() const;
        void SetColor(const Color& color);

        const Color& GetOutlineColor() const;
        void SetOutlineColor(const Color& outlineColor);

        const f32 GetOutlineWidth() const;
        void SetOutlineWidth(f32 outlineWidth);

        bool IsMultiline();
        void SetMultiline(bool multiline);

        void SetHorizontalAlignment(UI::TextHorizontalAlignment alignment);
        void SetVerticalAlignment(UI::TextVerticalAlignment alignment);

        static Label* CreateLabel();
    };
}