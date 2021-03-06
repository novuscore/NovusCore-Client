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
        
        FBox texCoord = FBox{ 0.0f, 1.0f, 1.0f, 0.0f };
        std::string border = "";
        Box borderSize;
        Box borderInset;
        
        Box slicingOffset;
    };
}

namespace UIComponent
{
    struct Image
    {
    public:
        struct ImageConstantBuffer
        {
            Color color; // 16 
            UI::Box borderSize; // 16 bytes
            UI::Box borderInset; // 16 bytes
            UI::Box slicingOffset; // 16 bytes
            vec2 size ; // 8 bytes

            u8 padding[8] = {};
        };
        Image(){ }

        UI::ImageStylesheet style;
        Renderer::TextureID textureID = Renderer::TextureID::Invalid();
        Renderer::TextureID borderID = Renderer::TextureID::Invalid();
        Renderer::BufferID vertexBufferID = Renderer::BufferID::Invalid();
        Renderer::Buffer<ImageConstantBuffer>* constantBuffer = nullptr;
    };
}