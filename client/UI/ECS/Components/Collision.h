#pragma once
#include <NovusTypes.h>

namespace UI
{
    enum CollisionFlags : u8
    {
        COLLISION = 1 << 0,
        INCLUDE_CHILDBOUNDS = 1 << 1
    };
}

namespace UIComponent
{
    struct Collision
    {
        Collision() {}

        u8 flags = 0;
        hvec2 minBound = hvec2(0.f, 0.f);
        hvec2 maxBound = hvec2(0.f, 0.f);

        inline void ToggleFlag(const UI::CollisionFlags inFlags) { flags ^= inFlags; }
        inline void SetFlag(const UI::CollisionFlags inFlags) { flags |= inFlags; }
        inline void UnsetFlag(const UI::CollisionFlags inFlags) { flags &= ~inFlags; }
        inline bool HasFlag(const UI::CollisionFlags inFlags) const { return (flags & inFlags) == inFlags; }
    };
}
