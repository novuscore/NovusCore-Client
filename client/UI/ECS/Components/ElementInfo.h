#pragma once
#include "../../UITypes.h"

namespace UIComponent
{
    struct ElementInfo
    {
        UI::ElementType type = UI::ElementType::NONE;
        void* scriptingObject = nullptr;
    };
}
