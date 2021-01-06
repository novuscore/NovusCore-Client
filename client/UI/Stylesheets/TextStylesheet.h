#pragma once
#include <NovusTypes.h>
#include "../UITypes.h"

namespace UI
{
    struct TextStylesheet
    {
        static void RegisterType();

        enum OverrideMaskProperties : u16
        {
            FONT_PATH = 1 << 0,
            FONT_SIZE = 1 << 1,
            LINE_HEIGHT_MULTIPLIER = 1 << 2,

            COLOR = 1 << 3,
            OUTLINE_COLOR = 1 << 4,
            OUTLINE_WIDTH = 1 << 5,

            HORIZONTAL_ALIGNMENT = 1 << 6,
            VERTICAL_ALIGNMENT = 1 << 7,

            MULTILINE = 1 << 8
        };

        u16 overrideMask = 0;

        std::string fontPath = "";
        f32 fontSize = 0;
        f32 lineHeightMultiplier = 1.15f;

        Color color = Color(1, 1, 1, 1);
        Color outlineColor = Color(0, 0, 0, 0);
        f32 outlineWidth = 0.f;

        UI::TextHorizontalAlignment horizontalAlignment = UI::TextHorizontalAlignment::LEFT;
        UI::TextVerticalAlignment verticalAlignment = UI::TextVerticalAlignment::TOP;
        bool multiline = false;

        inline void SetFontPath(std::string newFontPath) { fontPath = newFontPath; overrideMask |= FONT_PATH; }
        inline void SetFontSize(f32 newFontSize) { fontSize = newFontSize; overrideMask |= FONT_SIZE; }
        inline void SetLineHeightMultiplier(f32 newLineHeightMultiplier) { lineHeightMultiplier = newLineHeightMultiplier; overrideMask |= LINE_HEIGHT_MULTIPLIER; }

        inline void SetColor(Color newColor) { color = newColor; overrideMask |= COLOR; }
        inline void SetOutlineColor(Color newOutlineColor) { outlineColor = newOutlineColor; overrideMask |= OUTLINE_COLOR; }
        inline void SetOutlineWidth(f32 newOutlineWidth) { outlineWidth = newOutlineWidth; overrideMask |= OUTLINE_WIDTH; }

        inline void SetHorizontalAlignment(UI::TextHorizontalAlignment newHorizontalAlignment) { horizontalAlignment = newHorizontalAlignment; overrideMask |= HORIZONTAL_ALIGNMENT; }
        inline void SetVerticalAlignment(UI::TextVerticalAlignment newVerticalAlignment) { verticalAlignment = newVerticalAlignment; overrideMask |= VERTICAL_ALIGNMENT; }

        inline void SetMultiline(bool newMultiline) { multiline = newMultiline; overrideMask |= MULTILINE; }
    };
}
