#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/FrameResource.h>

#include "RenderResources.h"

namespace Renderer
{
    class Renderer;
    class RenderGraph;
    class DescriptorSet;
}

class PostProcessRenderer
{
public:
    PostProcessRenderer(Renderer::Renderer* renderer);
    ~PostProcessRenderer();

    void Update(f32 deltaTime);

    void AddCalculateSAOPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex);
    void AddPostProcessPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex);

    Renderer::ImageID GetAOImage(u32 frameIndex) { return _aoImage; }

private:
    void CreatePermanentResources();

private:
    Renderer::Renderer* _renderer;

    Renderer::ImageID _aoImage;
    Renderer::SamplerID _linearSampler;
};