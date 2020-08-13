#pragma once
#include <NovusTypes.h>
#include "../Descriptors/ModelDesc.h"

namespace ImGui {

    class ImDrawData;
}
namespace Renderer
{
    namespace Commands
    {
        struct DrawImgui
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;
        };
    }
}