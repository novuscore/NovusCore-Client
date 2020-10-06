#pragma once
#include "../../UITypes.h"

namespace UIComponent
{
    struct ElementInfo
    {
        UI::ElementType type = UI::ElementType::UITYPE_NONE;
        void* scriptingObject = nullptr;
    };
}
