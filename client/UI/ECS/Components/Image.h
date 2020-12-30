#pragma once
#include <NovusTypes.h>
#include <Renderer/Renderer.h>
#include <Renderer/Buffer.h>

namespace UI
{
    struct ImageStylesheet
    {
        enum OverrideMaskProperties : u8
        {
            TEXTURE = 1 << 0,
            COLOR = 1 << 1,
            
            TEXCOORD = 1 << 2,
            
            BORDER_TEXTURE = 1 << 3,
            BORDER_SIZE = 1 << 4,
            BORDER_INSET = 1 << 5,

            SLICING_OFFSET = 1 << 6,
        };

        u8 overrideMask = 0;

        std::string texture = "";
        FBox texCoord = FBox{ 0.0f, 1.0f, 1.0f, 0.0f };
        Color color = Color(1, 1, 1, 1);
        
        std::string borderTexture = "";
        Box borderSize;
        Box borderInset;
        
        Box slicingOffset;

        inline void SetTexture(std::string_view newTexture) { texture = newTexture; overrideMask |= TEXTURE; }
        inline void SetTexCoord(FBox newTexCoord) { texCoord = newTexCoord; overrideMask |= TEXCOORD; }
        inline void SetColor(Color newColor) { color = newColor; overrideMask |= COLOR; }

        inline void SetBorderTexture(std::string_view newBorderTexture) { borderTexture = newBorderTexture; overrideMask |= BORDER_TEXTURE; }
        inline void SetBorderSize(Box newBorderSize) { borderSize = newBorderSize; overrideMask |= BORDER_SIZE; }
        inline void SetBorderInset(Box newBorderInset) { borderInset = newBorderInset; overrideMask |= BORDER_INSET; }

        inline void SetSlicingOffset(Box newSlicingOffset) { slicingOffset = newSlicingOffset; overrideMask |= SLICING_OFFSET; }
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