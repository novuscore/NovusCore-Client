#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
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
                u16 depth;
                UI::DepthLayer depthLayer;
            } data { entt::null, 0, UI::DepthLayer::MEDIUM };
            u64 key;
        };
    };
}
