#pragma once
#include <NovusTypes.h>
#include <Renderer/Renderer.h>
#include <vector>

enum class TextAlignment
{
    LEFT,
    CENTER,
    RIGHT
};

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
    UIText() { }

    std::string text = "";
    u32 glyphCount = 0;

    Color color = Color(1,1,1,1);
    Color outlineColor = Color(0,0,0,0);
    f32 outlineWidth = 0.f;
    
    TextAlignment textAlignment = TextAlignment::LEFT;

    std::string fontPath = "";
    f32 fontSize = 0;
    Renderer::Font* font = nullptr;

    std::vector<Renderer::ModelID> models;
    std::vector<Renderer::TextureID> textures;

    Renderer::ConstantBuffer<TextConstantBuffer>* constantBuffer = nullptr;
};