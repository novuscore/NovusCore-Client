#pragma once
#include <NovusTypes.h>
#include <Renderer/Renderer.h>
#include <vector>

struct UIText
{
public:
    struct TextConstantBuffer
    {
        Color textColor; // 16 bytes
        Color outlineColor; // 16 bytes
        f32 outlineWidth; // 4 bytes

        u8 padding[220] = {};
    };

public:
    UIText() : text(), glyphCount(), color(1, 1, 1, 1), outlineColor(0, 0, 0, 1), outlineWidth(0.0f), fontPath(), fontSize(), font(), models(), textures(), constantBuffer(nullptr) { }

    std::string text;
    u32 glyphCount;

    Color color;
    Color outlineColor;
    f32 outlineWidth;

    std::string fontPath;
    f32 fontSize;
    Renderer::Font* font;

    std::vector<Renderer::ModelID> models;
    std::vector<Renderer::TextureID> textures;

    Renderer::ConstantBuffer<TextConstantBuffer>* constantBuffer;
};