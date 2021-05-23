#pragma once
#include <NovusTypes.h>
#include "../../../UI/UITypes.h"

namespace UI
{
    enum TransformFlags : u8
    {
        FILL_PARENTSIZE_X = 1 << 0,
        FILL_PARENTSIZE_Y = 1 << 1,

        FILL_PARENTSIZE = FILL_PARENTSIZE_X | FILL_PARENTSIZE_Y
    };
}

namespace UIComponent
{
    struct Transform
    {
        Transform() { }

        u8 flags = 0;

        hvec2 anchorPosition = hvec2(0.f, 0.f);
        hvec2 position = hvec2(0.f, 0.f);

        hvec2 anchor = hvec2(0.f, 0.f);
        hvec2 localAnchor = hvec2(0.f, 0.f);
        
        hvec2 size = hvec2(0.f, 0.f);
        
        UI::HBox padding;

        inline void ToggleFlag(const UI::TransformFlags inFlags) { flags ^= inFlags; }
        inline void SetFlag(const UI::TransformFlags inFlags) { flags |= inFlags; }
        inline void UnsetFlag(const UI::TransformFlags inFlags) { flags &= ~inFlags; }
        inline bool HasFlag(const UI::TransformFlags inFlags) const { return (flags & inFlags) == inFlags; }
    };
}