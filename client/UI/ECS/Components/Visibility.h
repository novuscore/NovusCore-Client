#pragma once

namespace UI
{
    enum VisibilityFlags : u8
    {
        VISIBLE = 1 << 0,
        PARENTVISIBLE = 1 << 1,

        FULL_VISIBLE = VISIBLE | PARENTVISIBLE
    };
}
namespace UIComponent
{
    struct Visibility
    {
        Visibility() {}

        u8 visibilityFlags = UI::VisibilityFlags::FULL_VISIBLE;

        void ToggleFlag(UI::VisibilityFlags flag) { visibilityFlags ^= flag; }
        bool HasFlag(UI::VisibilityFlags flag) const { return (visibilityFlags & flag) == flag; }
    };
}
