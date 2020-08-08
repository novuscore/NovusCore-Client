#pragma once
#include <NovusTypes.h>
#include <Renderer/Renderer.h>
#include <vector>

namespace UI
{
    enum class TextHorizontalAlignment : u8
    {
        LEFT,
        CENTER,
        RIGHT
    };

    enum class TextVerticalAlignment : u8
    {
        TOP,
        CENTER,
        BOTTOM
    };
}

namespace UIComponent
{
    struct Text
    {
    public:
        struct TextConstantBuffer
        {
            Color textColor = Color(); // 16 bytes
            Color outlineColor = Color(); // 16 bytes
            f32 outlineWidth = 0.f; // 4 bytes

            u8 padding[220] = {};
        };

    public:
        Text() { }

        std::string text = "";
        size_t glyphCount = 0;
        size_t pushback = 0;

        Color color = Color(1, 1, 1, 1);
        Color outlineColor = Color(0, 0, 0, 0);
        f32 outlineWidth = 0.f;


        UI::TextHorizontalAlignment horizontalAlignment = UI::TextHorizontalAlignment::LEFT;
        UI::TextVerticalAlignment verticalAlignment = UI::TextVerticalAlignment::TOP;
        bool isMultiline = false;

        f32 lineHeight = 1.15f;

        std::string fontPath = "";
        f32 fontSize = 0;
        Renderer::Font* font = nullptr;

        std::vector<Renderer::ModelID> models;
        std::vector<Renderer::TextureID> textures;

        Renderer::ConstantBuffer<TextConstantBuffer>* constantBuffer = nullptr;
    };
}