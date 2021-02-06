#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/DescriptorSet.h>

namespace Renderer
{
    class RenderGraph;
    class Renderer;
}
class DebugRenderer;

class Window;
class Keybind;
class UIRenderer
{
public:
    UIRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer);

    void Update(f32 deltaTime);

    void AddUIPass(Renderer::RenderGraph* renderGraph, Renderer::ImageID renderTarget, u8 frameIndex);

    void AddImguiPass(Renderer::RenderGraph* renderGraph, Renderer::ImageID renderTarget, u8 frameIndex);

private:
    void CreatePermanentResources();

private:
    Renderer::Renderer* _renderer;
    DebugRenderer* _debugRenderer;

    Renderer::TextureID _emptyBorder;

    Renderer::SamplerID _linearSampler;
    Renderer::BufferID _indexBuffer;

    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _drawImageDescriptorSet;
    Renderer::DescriptorSet _drawTextDescriptorSet;

};