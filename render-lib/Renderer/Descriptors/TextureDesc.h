#pragma once
#include <NovusTypes.h>
#include <Utils/StrongTypedef.h>
#include "../RenderStates.h"

namespace Renderer
{
    struct TextureDesc
    {
        std::string path = "";
    };

    struct DataTextureDesc
    {
        i32 width = 0;
        i32 height = 0;

        ImageFormat format;
        
        u8* data = nullptr;
        std::string debugName = "";
    };

    // Lets strong-typedef an ID type with the underlying type of u16
    STRONG_TYPEDEF(TextureID, u16);
}