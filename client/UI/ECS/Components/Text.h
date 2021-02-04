#pragma once
#include <NovusTypes.h>
#include "../../UITypes.h"
#include <Renderer/Renderer.h>
#include <Renderer/Buffer.h>
#include <Renderer/Font.h>
#include <vector>

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

        UI::TextStylesheet style;

        UI::TextHorizontalAlignment horizontalAlignment = UI::TextHorizontalAlignment::LEFT;
        UI::TextVerticalAlignment verticalAlignment = UI::TextVerticalAlignment::TOP;
        bool multiline = false;

        Renderer::Font* font = nullptr;

        size_t vertexBufferGlyphCount = 0;
        Renderer::BufferID vertexBufferID = Renderer::BufferID::Invalid();
        Renderer::BufferID textureIDBufferID = Renderer::BufferID::Invalid();
        Renderer::Buffer<TextConstantBuffer>* constantBuffer = nullptr;
    };
}