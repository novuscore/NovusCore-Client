#pragma once
#include <NovusTypes.h>
#include <vector>
#include <entity/entity.hpp>
#include "../../../UI/UITypes.h"

namespace UI
{
    struct UIChild
    {
        entt::entity entId;
        UI::UIElementType type;
    };

    enum TransformFlags : u8
    {
        FILL_PARENTSIZE = 1 << 0
    };
}

namespace UIComponent
{
    struct Transform
    {
        Transform()
        {
            children.reserve(8);
        }

        hvec2 position = hvec2(0.f, 0.f);
        hvec2 localPosition = hvec2(0.f, 0.f);
        hvec2 anchor = hvec2(0.f, 0.f);
        hvec2 localAnchor = hvec2(0.f, 0.f);
        hvec2 size = hvec2(0.f, 0.f);
        u8 flags = 0;
        entt::entity parent = entt::null;
        std::vector<UI::UIChild> children;
        void* asObject = nullptr;

        inline void SetFlag(const UI::TransformFlags inFlags) { flags |= inFlags; }
        inline void UnsetFlag(const UI::TransformFlags inFlags) { flags &= ~inFlags; }
        inline bool HasFlag(const UI::TransformFlags inFlags) { return (flags & inFlags) == inFlags; }
    };
}