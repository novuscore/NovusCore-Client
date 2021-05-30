#pragma once
#include <NovusTypes.h>
#include "../../../UI/UITypes.h"

namespace UI
{
    enum TransformFillFlags : u8
    {
        FILL_PARENTSIZE_X = 1 << 0,
        FILL_PARENTSIZE_Y = 1 << 1,

        FILL_PARENTSIZE = FILL_PARENTSIZE_X | FILL_PARENTSIZE_Y
    };
}

namespace UIComponent
{
    struct TransformFill
    {
        TransformFill() { }

        u8 flags = 0;
        UI::FBox fill = { 0.f, 1.f, 1.f, 0.f };

        inline void ToggleFlag(const UI::TransformFillFlags inFlags) { flags ^= inFlags; }
        inline void SetFlag(const UI::TransformFillFlags inFlags) { flags |= inFlags; }
        inline void UnsetFlag(const UI::TransformFillFlags inFlags) { flags &= ~inFlags; }
        inline bool HasFlag(const UI::TransformFillFlags inFlags) const { return (flags & inFlags) == inFlags; }
    };
}