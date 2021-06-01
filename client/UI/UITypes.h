#pragma once
#include <NovusTypes.h>

namespace UI
{
    enum class ElementType : u8
    {
        NONE,

        PANEL,
        BUTTON,
        CHECKBOX,
        SLIDER,
        SLIDERHANDLE,
        PROGRESSBAR,

        LABEL,
        INPUTFIELD
    };

    static char* GetElementTypeAsString(ElementType type)
    {
        switch (type)
        {
        case ElementType::NONE:
            return "None";
        case ElementType::PANEL:
            return "Panel";
        case ElementType::BUTTON:
            return "Button";
        case ElementType::CHECKBOX:
            return "Checkbox";
        case ElementType::SLIDER:
            return "Slider";
        case ElementType::SLIDERHANDLE:
            return "Slider Handle";
        case ElementType::PROGRESSBAR:
            return "Progress Bar";

        case ElementType::LABEL:
            return "Label";
        case ElementType::INPUTFIELD:
            return "Inputfield";
        default:
            DebugHandler::PrintFatal("ElementType has no valid string translation.");
            return "Unknown";
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
    
    struct UIVertex
    {
        vec2 pos;
        vec2 uv;
    };
}
