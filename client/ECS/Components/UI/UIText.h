#pragma once
#include <NovusTypes.h>
#include <Renderer/Renderer.h>
#include <vector>

namespace UI
{
    enum class TextAlignment
    {
        LEFT,
        CENTER,
        RIGHT
    };

    enum class TextType
    {
        SINGLELINE,
        MULTILINE
    };

    static float GetTextAlignment(TextAlignment alignment)
    {
        switch (alignment)
        {
        case TextAlignment::LEFT:
            return 0.f;
        case TextAlignment::CENTER:
            return 0.5f;
        case TextAlignment::RIGHT:
            return 1.f;
        default:
            assert(false); // We should never get here.
            return 0.f;
        }
    }

    struct UIText
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
        UIText() { }

        std::string text = "";
        size_t glyphCount = 0;

        Color color = Color(1, 1, 1, 1);
        Color outlineColor = Color(0, 0, 0, 0);
        f32 outlineWidth = 0.f;

        TextAlignment textAlignment = TextAlignment::LEFT;
        TextType textType = TextType::MULTILINE;

        f32 lineHeight = 1.15f;

        std::string fontPath = "";
        f32 fontSize = 0;
        Renderer::Font* font = nullptr;

        std::vector<Renderer::ModelID> models;
        std::vector<Renderer::TextureID> textures;

        Renderer::ConstantBuffer<TextConstantBuffer>* constantBuffer = nullptr;
    };
}