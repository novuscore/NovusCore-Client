#include "PostProcessRenderer.h"
#include "RenderUtils.h"

#include <Renderer/Renderer.h>
#include <Renderer/RenderGraph.h>
#include <CVar/CVarSystem.h>

PostProcessRenderer::PostProcessRenderer(Renderer::Renderer* renderer)
    : _renderer(renderer)
{
    CreatePermanentResources();
}

PostProcessRenderer::~PostProcessRenderer()
{

}

void PostProcessRenderer::Update(f32 deltaTime)
{

}

void PostProcessRenderer::AddPostProcessPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID colorTarget, Renderer::ImageID objectTarget, Renderer::DepthImageID depthTarget, Renderer::ImageID occlusionPyramid, u8 frameIndex)
{
    
}

void PostProcessRenderer::CreatePermanentResources()
{
    Renderer::SamplerDesc samplerDesc;
    samplerDesc.filter = Renderer::SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR;
    samplerDesc.addressU = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.addressV = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.minLOD = 0.f;
    samplerDesc.maxLOD = 16.f;
    samplerDesc.mode = Renderer::SamplerReductionMode::MIN;
    
    _linearSampler = _renderer->CreateSampler(samplerDesc);
}
