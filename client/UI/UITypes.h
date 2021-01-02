#pragma once
#include <NovusTypes.h>

namespace UI
{
    enum class ElementType : u8
    {
        UITYPE_NONE,

        UITYPE_PANEL,
        UITYPE_BUTTON,
        UITYPE_CHECKBOX,
        UITYPE_SLIDER,
        UITYPE_SLIDERHANDLE,

        UITYPE_LABEL,
        UITYPE_INPUTFIELD
    };

    static std::string GetElementTypeAsString(ElementType type)
    {
        switch (type)
        {
        case ElementType::UITYPE_NONE:
            return "None";
        case ElementType::UITYPE_PANEL:
            return "Panel";
        case ElementType::UITYPE_BUTTON:
            return "Button";
        case ElementType::UITYPE_CHECKBOX:
            return "Checkbox";
        case ElementType::UITYPE_SLIDER:
            return "Slider";
        case ElementType::UITYPE_SLIDERHANDLE:
            return "Slider Handle";
        case ElementType::UITYPE_LABEL:
            return "Label";
        case ElementType::UITYPE_INPUTFIELD:
            return "Inputfield";
        default:
            assert(false);
            return "Unimplemented";
        }
    }

    enum class DepthLayer : u16
    {
        WORLD,
        BACKGROUND,
        LOW,
        MEDIUM,
        HIGH,
        DIALOG,
        FULLSCREEN,
        FULLSCREEN_DIALOG,
        TOOLTIP,
        MAX
    };

    // Text
    enum class TextHorizontalAlignment : u8
    {
        LEFT,
        CENTER,
        RIGHT
    };

    enum class TextVerticalAlignment : u8
    {
        TOP,
        CENTER,
        BOTTOM
    };

#pragma pack(push, 1)
    struct Box
    {
        u32 top = 0;
        u32 right = 0;
        u32 bottom = 0;
        u32 left = 0;
    };
    struct FBox
    {
        f32 top = 0.0f;
        f32 right = 0.0f;
        f32 bottom = 0.0f;
        f32 left = 0.0f;
    };
    struct HBox
    {
        f16 top = f16(0.0f);
        f16 right = f16(0.0f);
        f16 bottom = f16(0.0f);
        f16 left = f16(0.0f);
    };
#pragma pack(pop)

    struct TextStylesheet
    {
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
