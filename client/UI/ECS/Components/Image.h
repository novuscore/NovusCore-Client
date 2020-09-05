#pragma once
#include <NovusTypes.h>
#include <Renderer/Renderer.h>
#include <Renderer/Buffer.h>

namespace UI
{
    struct ImageStylesheet
    {
        std::string texture = "";
        Color color = Color(1, 1, 1, 1);
    };
}

namespace UIComponent
{
    struct Image
    {
    public:
        struct ImageConstantBuffer
        {
            Color color; // 16 bytes

            u8 padding[240] = {};
        };
        Image(){ }

        UI::ImageStylesheet style;
        Renderer::TextureID textureID = Renderer::TextureID::Invalid();
        Renderer::BufferID vertexBufferID = Renderer::BufferID::Invalid();
        Renderer::Buffer<ImageConstantBuffer>* constantBuffer = nullptr;
    };
}