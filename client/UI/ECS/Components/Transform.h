#pragma once
#include <NovusTypes.h>
#include "../../../UI/UITypes.h"

namespace UIComponent
{
    struct Transform
    {
        Transform() { }

        hvec2 anchorPosition = hvec2(0.f, 0.f);
        hvec2 position = hvec2(0.f, 0.f);

        hvec2 anchor = hvec2(0.f, 0.f);
        hvec2 localAnchor = hvec2(0.f, 0.f);
        
        hvec2 size = hvec2(0.f, 0.f);
        
        UI::HBox padding;
    };
}