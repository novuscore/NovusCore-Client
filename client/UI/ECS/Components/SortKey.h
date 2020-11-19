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
#pragma pack(push, 1)
            struct
            {
                u16 childOrder;
                u16 subDepth;
                u16 depth;
                UI::DepthLayer depthLayer;
            } data { 0, 0, 0, UI::DepthLayer::MEDIUM };
#pragma pack(pop)
            u64 key;
        };
    };
}
