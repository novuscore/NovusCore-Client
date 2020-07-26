#pragma once
#include <NovusTypes.h>

#include "../../../ECS/Components/UI/UIText.h"
#include "asUITransform.h"

namespace UI
{
    class asLabel : public asUITransform
    {
    public:
        asLabel(entt::entity entityId);

        static void RegisterType();

        //Text Functions
        void SetText(const std::string& text);
        const std::string GetText() const { return _text.text; }

        void SetFont(const std::string& fontPath, f32 fontSize);

        void SetColor(const Color& color);
        const Color& GetColor() const { return _text.color; }

        void SetOutlineColor(const Color& outlineColor);
        const Color& GetOutlineColor() const { return _text.outlineColor; }

        void SetOutlineWidth(f32 outlineWidth);
        const f32 GetOutlineWidth() const { return _text.outlineWidth; }

        void SetHorizontalAlignment(TextHorizontalAlignment alignment);
        void SetVerticalAlignment(TextVerticalAlignment alignment);

        static asLabel* CreateLabel();

    private:
        UIText _text;
    };
}