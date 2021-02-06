#include "CullUtils.h"

#include <Renderer/Renderer.h>
#include <Renderer/RenderGraphResources.h>
#include <Renderer/CommandList.h>

#include <Renderer/Descriptors/ComputeShaderDesc.h>
#include <Renderer/Descriptors/ComputePipelineDesc.h>

Renderer::DescriptorSet DepthPyramidUtils::_reduceDescriptorSet;

inline u32 GetGroupCount(u32 threadCount, u32 localSize)
{
    return (threadCount + localSize - 1) / localSize;
}

void DepthPyramidUtils::BuildPyramid(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, Renderer::DepthImageID depthImage, Renderer::ImageID pyramidImage)
{
    Renderer::ComputePipelineDesc queryPipelineDesc;
    resources.InitializePipelineDesc(queryPipelineDesc);

    Renderer::ComputeShaderDesc shaderDesc;
    shaderDesc.path = "Blitting/blitDepth.cs.hlsl";
    queryPipelineDesc.computeShader = renderer->LoadShader(shaderDesc);

    // Do culling
    Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(queryPipelineDesc);
    commandList.BeginPipeline(pipeline);

    commandList.PushMarker("Depth Pyramid ", Color::White);

    Renderer::ImageDesc pyramidInfo = renderer->GetImageDesc(pyramidImage);
    Renderer::DepthImageDesc depthInfo = renderer->GetDepthImageDesc(depthImage);
    uvec2 pyramidSize = renderer->GetImageDimension(pyramidImage);

    Renderer::SamplerDesc samplerDesc;
    samplerDesc.filter = Renderer::SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR;

    samplerDesc.addressU = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.addressV = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.minLOD = 0.f;
    samplerDesc.maxLOD = 16.f;
    samplerDesc.mode = Renderer::SamplerReductionMode::MIN;

    Renderer::SamplerID occlusionSampler = renderer->CreateSampler(samplerDesc);

    for (uint32_t i = 0; i < pyramidInfo.mipLevels; ++i)
    {
        _reduceDescriptorSet.Bind("_sampler", occlusionSampler);
        _reduceDescriptorSet.BindStorage("_target", pyramidImage, i);

        if (i == 0)
        {
            _reduceDescriptorSet.Bind("_source", depthImage);
        }
        else 
        {
            _reduceDescriptorSet.Bind("_source", pyramidImage, i - 1);
        }

        u32 levelWidth = pyramidSize.x >> i;
        u32 levelHeight = pyramidSize.y >> i;
        if (levelHeight < 1) levelHeight = 1;
        if (levelWidth < 1) levelWidth = 1;

        struct alignas(16) DepthReduceParams
        {
            glm::vec2 imageSize;
            u32 level;
            u32 dummy;
        };

        DepthReduceParams* reduceData = resources.FrameNew<DepthReduceParams>();
        reduceData->imageSize = glm::vec2(levelWidth, levelHeight);
        reduceData->level = i;

        commandList.PushConstant(reduceData, 0, sizeof(DepthReduceParams));

        commandList.BindDescriptorSet(Renderer::GLOBAL, &_reduceDescriptorSet, frameIndex);
        commandList.Dispatch(GetGroupCount(levelWidth, 32), GetGroupCount(levelHeight, 32), 1);

        commandList.ImageBarrier(pyramidImage);
    }

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}
