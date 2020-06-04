#pragma once
#include <NovusTypes.h>
#include <Utils/StrongTypedef.h>

namespace Renderer
{
    struct TextureArrayDesc
    {
        u32 size;
    };

    // Lets strong-typedef an ID type with the underlying type of u16
    STRONG_TYPEDEF(TextureArrayID, u16);
}