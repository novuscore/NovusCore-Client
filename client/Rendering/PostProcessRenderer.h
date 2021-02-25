#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>

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

    void AddPostProcessPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID colorTarget, Renderer::ImageID objectTarget, Renderer::DepthImageID depthTarget, Renderer::ImageID occlusionPyramid, u8 frameIndex);

private:
    void CreatePermanentResources();

private:
    Renderer::Renderer* _renderer;

    Renderer::SamplerID _linearSampler;
};