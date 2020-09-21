#pragma once
#include <NovusTypes.h>
#include <entity/entity.hpp>
#include "../../../UI/UITypes.h"

namespace UIComponent
{
    struct SortKey
    {
        union
        {
            struct
            {
                entt::entity entId;
                UI::UIElementType type;
                u16 depth;
                UI::DepthLayer depthLayer;
            } data{ entt::null, UI::UIElementType::UITYPE_NONE, 0, UI::DepthLayer::MEDIUM };
            u64 key;
        };
    };
}
