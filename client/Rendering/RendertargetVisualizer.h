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

class RendertargetVisualizer
{
public:
    RendertargetVisualizer(Renderer::Renderer* renderer);
    ~RendertargetVisualizer();

    void Update(f32 deltaTime);
    void DrawImgui();

    void SetVisible(bool visible) { _isVisible = visible; }

    void AddVisualizerPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID colorTarget, u8 frameIndex);

    bool GetOverridingImageID(Renderer::ImageID& imageID);
    bool GetOverridingDepthImageID(Renderer::DepthImageID& imageID);

private:
    void CreatePermanentResources();

private:
    bool _isVisible = false;
    Renderer::Renderer* _renderer;

    i32 _selectedOverrideMip = 0;

    vec4 _additiveColor = vec4(0,0,0,0);
    vec4 _colorMultiplier = vec4(1,1,1,1);
    i32 _selectedOverlayMip = 0;

    Renderer::ImageID _overridingImageID;
    Renderer::DepthImageID _overridingDepthImageID;

    Renderer::ImageID _overlayingImageID;
    Renderer::DepthImageID _overlayingDepthImageID;

    Renderer::SamplerID _linearSampler;
};