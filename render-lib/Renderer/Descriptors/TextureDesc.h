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
        TextureType type = TEXTURE_TYPE_2D;

        i32 width = 0;
        i32 height = 0;
        i32 layers = 1;

        ImageFormat format;
        
        u8* data = nullptr;
        std::string debugName = "";
    };

    // Lets strong-typedef an ID type with the underlying type of u16
    STRONG_TYPEDEF(TextureID, u16);
}