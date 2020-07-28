#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/DescriptorSet.h>

namespace Renderer
{
    class RenderGraph;
    class Renderer;
}

class Window;
class Keybind;
class UIRenderer
{
public:
    UIRenderer(Renderer::Renderer* renderer);

    void AddUIPass(Renderer::RenderGraph* renderGraph, Renderer::ImageID renderTarget, u8 frameIndex);

private:
    void CreatePermanentResources();

private:
    Renderer::Renderer* _renderer;

    Renderer::SamplerID _linearSampler;

    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _drawDescriptorSet;
};