#pragma once
#include <NovusTypes.h>

namespace UI
{
    enum class UIElementType : u8
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
}
