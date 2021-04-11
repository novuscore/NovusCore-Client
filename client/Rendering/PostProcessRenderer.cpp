#include "PostProcessRenderer.h"
#include "RenderUtils.h"
#include "PostProcess/SAO.h"
#include "../Utils/ServiceLocator.h"
#include "Camera.h"

#include <Renderer/Renderer.h>
#include <Renderer/RenderGraph.h>
#include <CVar/CVarSystem.h>
#include <glm/gtx/euler_angles.hpp>

AutoCVar_Int CVAR_SAOEnabled("SAO.enabled", "Enable SAO", 1, CVarFlags::EditCheckbox);
AutoCVar_Float CVAR_SAORadius("SAO.radius", "SAO radius", 0.4f);
AutoCVar_Float CVAR_SAOBias("SAO.bias", "SAO bias", 0.03f);
AutoCVar_Float CVAR_SAOIntensity("SAO.intensity", "SAO intensity", 0.5f);

PostProcessRenderer::PostProcessRenderer(Renderer::Renderer* renderer)
    : _renderer(renderer)
{
    CreatePermanentResources();

    PostProcess::SAO::Init(renderer);
}

PostProcessRenderer::~PostProcessRenderer()
{

}

void PostProcessRenderer::Update(f32 deltaTime)
{

}

f32 CalculateProjScale(Camera* camera, vec2 resolution)
{
    float fov = 75.0f; // TODO: Don't hardcode FOV

    float scale = -2.0f * tan(glm::radians(fov) * 0.5f);

    return resolution.y / scale;
}

void PostProcessRenderer::AddCalculateSAOPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex)
{
    bool saoEnabled = CVAR_SAOEnabled.Get() == 1;

    struct CalculateSAOPassData
    {
    };

    renderGraph->AddPass<CalculateSAOPassData>("Calculate SAO",
        [=](CalculateSAOPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
    {
        return true; // Return true from setup to enable this pass, return false to disable it
    },
        [=](CalculateSAOPassData& data, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList) // Execute
    {
        if (saoEnabled)
        {
            Camera* camera = ServiceLocator::GetCamera();

            PostProcess::SAO::Params params;
            params.depth = resources.depth;

            vec2 resolution = _renderer->GetImageDimension(resources.color, 0);

            params.nearPlane = camera->GetNearClip();
            params.farPlane = camera->GetFarClip();

            params.projScale = CalculateProjScale(camera, resolution);
            params.radius = CVAR_SAORadius.GetFloat();
            params.bias = CVAR_SAOBias.GetFloat();
            params.intensity = CVAR_SAOIntensity.GetFloat();
            params.viewMatrix = camera->GetViewMatrix();
            params.invProjMatrix = glm::inverse(camera->GetProjectionMatrix());

            params.output = _aoImage;

            PostProcess::SAO::CalculateSAO(_renderer, graphResources, commandList, frameIndex, params);
        }
        else
        {
            commandList.Clear(_aoImage, Color(1, 1, 1, 1));
        }
    });
}

void PostProcessRenderer::AddPostProcessPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex)
{

}

void PostProcessRenderer::CreatePermanentResources()
{
    Renderer::ImageDesc imageDesc;
    imageDesc.dimensions = vec2(1.0f, 1.0f);
    imageDesc.dimensionType = Renderer::ImageDimensionType::DIMENSION_SCALE;
    imageDesc.format = Renderer::ImageFormat::R16G16B16A16_FLOAT;
    imageDesc.sampleCount = Renderer::SampleCount::SAMPLE_COUNT_1;

    imageDesc.debugName = "AmbientObscurance";
    _aoImage = _renderer->CreateImage(imageDesc);

    Renderer::SamplerDesc samplerDesc;
    samplerDesc.filter = Renderer::SamplerFilter::MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.addressU = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.addressV = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.minLOD = 0.f;
    samplerDesc.maxLOD = 16.f;
    samplerDesc.mode = Renderer::SamplerReductionMode::MIN;
    
    _linearSampler = _renderer->CreateSampler(samplerDesc);
}
