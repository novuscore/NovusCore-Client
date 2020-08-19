#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>
#include "Descriptors/TextureArrayDesc.h"
#include "Descriptors/FontDesc.h"

struct stbtt_fontinfo;

namespace Renderer
{
    class Renderer;

    struct FontChar
    {
        f32 advance;
        i32 xOffset;
        i32 yOffset;
        i32 width = 0;
        i32 height = 0;
        u8* data;

        u32 textureIndex;
    };

    struct Font
    {
        FontDesc desc;

        stbtt_fontinfo* fontInfo;
        float scale;

        FontChar& GetChar(char character);
        TextureArrayID GetTextureArray();

        static Font* GetFont(Renderer* renderer, const std::string& fontPath, f32 fontSize);
        
    private:
        Font() = default;

        bool InitChar(char character, FontChar& fontChar);

    private:
        static robin_hood::unordered_map<u64, Font*> _fonts;
        robin_hood::unordered_map<char, FontChar> _chars;

        TextureArrayID _textureArray = TextureArrayID::Invalid();

        Renderer* _renderer;

        friend class Renderer;
    };
}