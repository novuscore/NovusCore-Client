#pragma once
#include <NovusTypes.h>
#include "../../../UI/UITypes.h"

namespace UI
{
    struct UIChild
    {
        entt::entity entId;
        UI::UIElementType type;
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

        vec2 position = vec2(0, 0);
        vec2 localPosition = vec2(0, 0);
        vec2 anchor = vec2(0, 0);
        vec2 localAnchor = vec2(0, 0);
        vec2 size = vec2(0, 0);
        bool fillParentSize = false;
        union
        {
            u64 sortKey = 0;
            struct
            {
                u8 depthLayer;
                u16 depth;
                UI::UIElementType type;
                entt::entity entId;
            } sortData;
        };
        entt::entity parent = entt::null;
        std::vector<UI::UIChild> children;
        void* asObject = nullptr;

        vec2 minBound = vec2(0, 0);
        vec2 maxBound = vec2(0, 0);
        bool includeChildBounds = false;
    };
}