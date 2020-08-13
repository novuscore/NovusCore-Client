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
        Image() : texture(), textureID(Renderer::TextureID::Invalid()), modelID(Renderer::ModelID::Invalid()), color(1, 1, 1, 1), constantBuffer(nullptr) { }

        std::string texture;
        Renderer::TextureID textureID;
        Renderer::ModelID modelID;
        Color color;
        Renderer::Buffer<ImageConstantBuffer>* constantBuffer;
    };
}