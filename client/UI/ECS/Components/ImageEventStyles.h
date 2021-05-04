#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>
#include "../../Stylesheets/ImageStylesheet.h"
#include "TransformEvents.h"

namespace UIComponent
{
    struct ImageEventStyles
    {
    public:
        ImageEventStyles()
        {
            styleMap.reserve(UI::TransformEventState::COUNT);
        }

        robin_hood::unordered_flat_map<UI::TransformEventState, UI::ImageStylesheet> styleMap;
    };
}