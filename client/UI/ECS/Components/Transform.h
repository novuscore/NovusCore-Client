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
                UI::DepthLayer depthLayer;
                u16 depth;
                UI::UIElementType type;
                entt::entity entId;
            } sortData{ UI::DepthLayer::MEDIUM, 0, UI::UIElementType::UITYPE_NONE, entt::null };
            u64 sortKey;
        };
        entt::entity parent = entt::null;
        std::vector<UI::UIChild> children;
        void* asObject = nullptr;

        vec2 minBound = vec2(0, 0);
        vec2 maxBound = vec2(0, 0);
        bool includeChildBounds = false;
    };
}