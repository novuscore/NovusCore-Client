#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>
#include "../../Stylesheets/TextStylesheet.h"
#include "TransformEvents.h"

namespace UIComponent
{
    struct TextEventStyles
    {
    public:
        TextEventStyles() { }

        robin_hood::unordered_flat_map<UI::TransformEventState, UI::TextStylesheet> styleMap;
    };
}