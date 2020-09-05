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

    enum class DepthLayer : u8
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
            struct
            {
                entt::entity entId;
                UI::UIElementType type;
                u16 depth;
                UI::DepthLayer depthLayer;
            } sortData{ entt::null, UI::UIElementType::UITYPE_NONE, 0, UI::DepthLayer::MEDIUM };
            u64 sortKey;
        };
        entt::entity parent = entt::null;
        std::vector<UI::UIChild> children;
        void* asObject = nullptr;

        bool collision = true;
        vec2 minBound = vec2(0, 0);
        vec2 maxBound = vec2(0, 0);
        bool includeChildBounds = false;
    };
}