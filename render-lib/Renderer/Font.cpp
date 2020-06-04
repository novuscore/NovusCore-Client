#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "Font.h"
#include "Renderer.h"
#include <Utils/XXHash64.h>
#include <Utils/FileReader.h>
#include <Utils/DebugHandler.h>
#include <filesystem>

namespace Renderer
{
    robin_hood::unordered_map<u64, Font*> Font::_fonts;

    FontChar& Font::GetChar(char character)
    {
        auto it = _chars.find(character);
        if (it == _chars.end())
        {
            FontChar fontChar;
            if (!InitChar(character, fontChar))
            {
                NC_LOG_FATAL("The font does not support this character");
            }

            _chars[character] = fontChar;
        }

        return _chars[character];
    }

    Font* Font::GetFont(Renderer* renderer, const std::string& fontPath, f32 fontSize)
    {
        // Hash and add together the fontPath and fontSize
        u64 hash = XXHash64::hash(fontPath.data(), fontPath.size(), 42) + XXHash64::hash(&fontSize, sizeof(f32), 42);

        auto it = _fonts.find(hash);
        if (it == _fonts.end())
        {
            Font* font = new Font();
            font->_renderer = renderer;

            std::filesystem::path path = std::filesystem::absolute(fontPath);
            FileReader file(path.string(), path.filename().string());
            if (!file.Open())
            {
                NC_LOG_FATAL("Could not open Font file %s", fontPath.c_str());
            }

            std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<1048576>();
            file.Read(*buffer, file.Length());

            font->fontInfo = new stbtt_fontinfo();
            stbtt_InitFont(font->fontInfo, buffer->GetDataPointer(), 0);

            font->scale = stbtt_ScaleForPixelHeight(font->fontInfo, fontSize);

            // Preload char 32 to 127 (commonly used ASCII characters)
            for (int i = 32; i < 127; i++)
            {
                FontChar fontChar;
                if (font->InitChar(i, fontChar))
                {
                    font->_chars[i] = fontChar;
                }
            }
            
            _fonts[hash] = font;
        }

        return _fonts[hash];
    }

    bool Font::InitChar(char character, FontChar& fontChar)
    {
        fontChar.data = stbtt_GetCodepointSDF(fontInfo, scale, character, desc.padding, 128, 64.0f, &fontChar.width, &fontChar.height, &fontChar.xOffset, &fontChar.yOffset);

        if (fontChar.width == 0 && fontChar.height == 0)
            return false;

        int advance;
        stbtt_GetCodepointHMetrics(fontInfo, character, &advance, NULL);
        fontChar.advance = advance * scale;

        DataTextureDesc textureDesc;
        textureDesc.width = fontChar.width;
        textureDesc.height = fontChar.height;
        textureDesc.format = IMAGE_FORMAT_R8_UNORM;
        textureDesc.data = fontChar.data;
        textureDesc.debugName = desc.path + " " + character;

        fontChar.texture = _renderer->CreateDataTexture(textureDesc);

        return true;
    }
}