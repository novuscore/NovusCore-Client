#pragma once
#include <NovusTypes.h>
#include <Renderer/Renderer.h>

struct UIRenderable
{
public:
    struct RenderableConstantBuffer
    {
        Color color; // 16 bytes

        u8 padding[240] = {};
    };
    UIRenderable() : texture(), textureID(Renderer::TextureID::Invalid()), modelID(Renderer::ModelID::Invalid()), color(1, 1, 1, 1), constantBuffer(nullptr) { }

    std::string texture;
    Renderer::TextureID textureID;
    Renderer::ModelID modelID;
    Color color;
    Renderer::ConstantBuffer<RenderableConstantBuffer>* constantBuffer;
};