#pragma once
#include <NovusTypes.h>
#include <Renderer/Renderer.h>
#include <Renderer/Buffer.h>

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

        std::string texture = "";
        Renderer::TextureID textureID = Renderer::TextureID::Invalid();
        Renderer::BufferID vertexBufferID = Renderer::BufferID::Invalid();
        Color color = Color(1,1,1,1);
        Renderer::Buffer<ImageConstantBuffer>* constantBuffer = nullptr;
    };
}