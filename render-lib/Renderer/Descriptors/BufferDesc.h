#pragma once
#include <NovusTypes.h>
#include <Utils/StrongTypedef.h>
#include "../RenderStates.h"

namespace Renderer
{
    struct BufferDesc
    {
        std::string name = "";
        u8 usage;
        BufferCPUAccess cpuAccess = BufferCPUAccess::None;
        u64 size = 0;
    };

    // Lets strong-typedef an ID type with the underlying type of u16
    STRONG_TYPEDEF(BufferID, u16);
}