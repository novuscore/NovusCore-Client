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
}
