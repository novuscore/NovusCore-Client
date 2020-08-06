#pragma once
#include <NovusTypes.h>

#include "../ECS/Components/Text.h"
#include "BaseElement.h"

namespace UIScripting
{
    class Label : public BaseElement
    {
    public:
        Label();

        static void RegisterType();

        //Text Functions
        void SetText(const std::string& text);
        const std::string GetText() const { return _text->text; }

        void SetFont(const std::string& fontPath, f32 fontSize);

        void SetColor(const Color& color);
        const Color& GetColor() const { return _text->color; }

        void SetOutlineColor(const Color& outlineColor);
        const Color& GetOutlineColor() const { return _text->outlineColor; }

        void SetOutlineWidth(f32 outlineWidth);
        const f32 GetOutlineWidth() const { return _text->outlineWidth; }

        void SetHorizontalAlignment(UI::TextHorizontalAlignment alignment);
        void SetVerticalAlignment(UI::TextVerticalAlignment alignment);

        static Label* CreateLabel();

    private:
        UIComponent::Text* _text;
    };
}