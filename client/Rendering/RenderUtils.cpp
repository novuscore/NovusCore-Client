#include "RenderUtils.h"
#include <Renderer/Renderer.h>
#include <Renderer/RenderGraphResources.h>
#include <Renderer/CommandList.h>

#include <Renderer/Descriptors/VertexShaderDesc.h>
#include <Renderer/Descriptors/PixelShaderDesc.h>

Renderer::DescriptorSet RenderUtils::_overlayDescriptorSet;

void RenderUtils::Blit(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const BlitParams& params)
{
    commandList.PushMarker("Blit", Color::White);
    commandList.ImageBarrier(params.input);

    Renderer::ImageDesc imageDesc = renderer->GetImageDesc(params.input);

    // Setup pipeline
    Renderer::VertexShaderDesc vertexShaderDesc;
    vertexShaderDesc.path = "Blitting/blit.vs.hlsl";

    Renderer::ImageComponentType componentType = Renderer::ToImageComponentType(imageDesc.format);
    std::string componentTypeName = "";

    switch (componentType)
    {
        case Renderer::ImageComponentType::FLOAT:
        case Renderer::ImageComponentType::SNORM:
        case Renderer::ImageComponentType::UNORM:
            componentTypeName = "float";
            break;
        case Renderer::ImageComponentType::SINT:
            componentTypeName = "int";
            break;
        case Renderer::ImageComponentType::UINT:
            componentTypeName = "uint";
            break;
    }

    u8 componentCount = Renderer::ToImageComponentCount(imageDesc.format);
    if (componentCount > 1)
    {
        componentTypeName += std::to_string(componentCount);
    }

    Renderer::PixelShaderDesc pixelShaderDesc;
    pixelShaderDesc.path = "Blitting/blit.ps.hlsl";
    pixelShaderDesc.AddPermutationField("TEX_TYPE", componentTypeName);

    Renderer::GraphicsPipelineDesc pipelineDesc;
    graphResources.InitializePipelineDesc(pipelineDesc);

    pipelineDesc.states.vertexShader = renderer->LoadShader(vertexShaderDesc);
    pipelineDesc.states.pixelShader = renderer->LoadShader(pixelShaderDesc);

    pipelineDesc.renderTargets[0] = params.output;

    pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;
    pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

    Renderer::GraphicsPipelineID pipeline = renderer->CreatePipeline(pipelineDesc);
    commandList.BeginPipeline(pipeline);

    u32 mipLevel = params.inputMipLevel;
    if (mipLevel >= imageDesc.mipLevels)
    {
        mipLevel = imageDesc.mipLevels - 1;
    }

    _overlayDescriptorSet.Bind("_sampler", params.sampler);
    _overlayDescriptorSet.Bind("_texture", params.input, mipLevel);
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, &_overlayDescriptorSet, frameIndex);

    struct BlitConstant
    {
        vec4 colorMultiplier;
        vec4 additiveColor;
        u32 channelRedirectors;
    };

    BlitConstant* constants = graphResources.FrameNew<BlitConstant>();
    constants->colorMultiplier = params.colorMultiplier;
    constants->additiveColor = params.additiveColor;

    u32 channelRedirectors = params.channelRedirectors.r;
    channelRedirectors |= (params.channelRedirectors.g << 8);
    channelRedirectors |= (params.channelRedirectors.b << 16);
    channelRedirectors |= (params.channelRedirectors.a << 24);

    constants->channelRedirectors = channelRedirectors;

    commandList.PushConstant(constants, 0, sizeof(BlitConstant));

    commandList.Draw(3, 1, 0, 0);

    commandList.EndPipeline(pipeline);
    commandList.ImageBarrier(params.input);
    commandList.PopMarker();
}

void RenderUtils::DepthBlit(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const DepthBlitParams& params)
{
    commandList.PushMarker("Blit", Color::White);
    commandList.ImageBarrier(params.input);

    Renderer::DepthImageDesc imageDesc = renderer->GetDepthImageDesc(params.input);

    // Setup pipeline
    Renderer::VertexShaderDesc vertexShaderDesc;
    vertexShaderDesc.path = "Blitting/blit.vs.hlsl";

    Renderer::ImageComponentType componentType = Renderer::ToImageComponentType(imageDesc.format);
    std::string componentTypeName = "";

    switch (componentType)
    {
    case Renderer::ImageComponentType::FLOAT:
    case Renderer::ImageComponentType::SNORM:
    case Renderer::ImageComponentType::UNORM:
        componentTypeName = "float";
        break;
    case Renderer::ImageComponentType::SINT:
        componentTypeName = "int";
        break;
    case Renderer::ImageComponentType::UINT:
        componentTypeName = "uint";
        break;
    }

    u8 componentCount = Renderer::ToImageComponentCount(imageDesc.format);
    if (componentCount > 1)
    {
        componentTypeName += std::to_string(componentCount);
    }

    Renderer::PixelShaderDesc pixelShaderDesc;
    pixelShaderDesc.path = "Blitting/blit.ps.hlsl";
    pixelShaderDesc.AddPermutationField("TEX_TYPE", componentTypeName);

    Renderer::GraphicsPipelineDesc pipelineDesc;
    graphResources.InitializePipelineDesc(pipelineDesc);

    pipelineDesc.states.vertexShader = renderer->LoadShader(vertexShaderDesc);
    pipelineDesc.states.pixelShader = renderer->LoadShader(pixelShaderDesc);

    pipelineDesc.renderTargets[0] = params.output;

    pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;
    pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

    Renderer::GraphicsPipelineID pipeline = renderer->CreatePipeline(pipelineDesc);
    commandList.BeginPipeline(pipeline);

    _overlayDescriptorSet.Bind("_sampler", params.sampler);
    _overlayDescriptorSet.Bind("_texture", params.input);
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, &_overlayDescriptorSet, frameIndex);

    struct BlitConstant
    {
        vec4 colorMultiplier;
        vec4 additiveColor;
        u32 channelRedirectors;
    };

    BlitConstant* constants = graphResources.FrameNew<BlitConstant>();
    constants->colorMultiplier = params.colorMultiplier;
    constants->additiveColor = params.additiveColor;

    u32 channelRedirectors = params.channelRedirectors.r;
    channelRedirectors |= (params.channelRedirectors.g << 8);
    channelRedirectors |= (params.channelRedirectors.b << 16);
    channelRedirectors |= (params.channelRedirectors.a << 24);

    constants->channelRedirectors = channelRedirectors;

    commandList.PushConstant(constants, 0, sizeof(BlitConstant));

    commandList.Draw(3, 1, 0, 0);

    commandList.EndPipeline(pipeline);
    commandList.ImageBarrier(params.input);
    commandList.PopMarker();
}

void RenderUtils::Overlay(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const OverlayParams& params)
{
    commandList.PushMarker("Overlay", Color::White);
    commandList.ImageBarrier(params.overlayImage);

    Renderer::ImageDesc imageDesc = renderer->GetImageDesc(params.overlayImage);

    // Setup pipeline
    Renderer::VertexShaderDesc vertexShaderDesc;
    vertexShaderDesc.path = "Blitting/blit.vs.hlsl";

    Renderer::ImageComponentType componentType = Renderer::ToImageComponentType(imageDesc.format);
    std::string componentTypeName = "";

    switch (componentType)
    {
    case Renderer::ImageComponentType::FLOAT:
    case Renderer::ImageComponentType::SNORM:
    case Renderer::ImageComponentType::UNORM:
        componentTypeName = "float";
        break;
    case Renderer::ImageComponentType::SINT:
        componentTypeName = "int";
        break;
    case Renderer::ImageComponentType::UINT:
        componentTypeName = "uint";
        break;
    }

    u8 componentCount = Renderer::ToImageComponentCount(imageDesc.format);
    if (componentCount > 1)
    {
        componentTypeName += std::to_string(componentCount);
    }

    Renderer::PixelShaderDesc pixelShaderDesc;
    pixelShaderDesc.path = "Blitting/blit.ps.hlsl";
    pixelShaderDesc.AddPermutationField("TEX_TYPE", componentTypeName);

    Renderer::GraphicsPipelineDesc pipelineDesc;
    graphResources.InitializePipelineDesc(pipelineDesc);

    pipelineDesc.states.vertexShader = renderer->LoadShader(vertexShaderDesc);
    pipelineDesc.states.pixelShader = renderer->LoadShader(pixelShaderDesc);

    pipelineDesc.renderTargets[0] = params.baseImage;

    pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;
    pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

    pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
    pipelineDesc.states.blendState.renderTargets[0].blendOp = Renderer::BlendOp::ADD;
    pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::SRC_ALPHA;
    pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::ONE;

    Renderer::GraphicsPipelineID pipeline = renderer->CreatePipeline(pipelineDesc);

    commandList.BeginPipeline(pipeline);

    u32 mipLevel = params.mipLevel;
    if (mipLevel >= imageDesc.mipLevels)
    {
        mipLevel = imageDesc.mipLevels - 1;
    }

    _overlayDescriptorSet.Bind("_sampler", params.sampler);
    _overlayDescriptorSet.Bind("_texture", params.overlayImage, mipLevel);
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, &_overlayDescriptorSet, frameIndex);

    struct BlitConstant
    {
        vec4 colorMultiplier;
        vec4 additiveColor;
        u32 channelRedirectors;
    };

    BlitConstant* constants = graphResources.FrameNew<BlitConstant>();
    constants->colorMultiplier = params.colorMultiplier;
    constants->additiveColor = params.additiveColor;

    u32 channelRedirectors = params.channelRedirectors.r;
    channelRedirectors |= (params.channelRedirectors.g << 8);
    channelRedirectors |= (params.channelRedirectors.b << 16);
    channelRedirectors |= (params.channelRedirectors.a << 24);

    constants->channelRedirectors = channelRedirectors;

    commandList.PushConstant(constants, 0, sizeof(BlitConstant));

    commandList.Draw(3, 1, 0, 0);

    commandList.EndPipeline(pipeline);
    commandList.ImageBarrier(params.overlayImage);
    commandList.PopMarker();
}

void RenderUtils::DepthOverlay(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const DepthOverlayParams& params)
{
    commandList.PushMarker("DepthOverlay", Color::White);
    commandList.ImageBarrier(params.overlayImage);

    // Setup pipeline
    Renderer::VertexShaderDesc vertexShaderDesc;
    vertexShaderDesc.path = "Blitting/blit.vs.hlsl";

    Renderer::DepthImageDesc imageDesc = renderer->GetDepthImageDesc(params.overlayImage);

    Renderer::ImageComponentType componentType = Renderer::ToImageComponentType(imageDesc.format);
    std::string componentTypeName = "";

    switch (componentType)
    {
        case Renderer::ImageComponentType::FLOAT:
        case Renderer::ImageComponentType::SNORM:
        case Renderer::ImageComponentType::UNORM:
            componentTypeName = "float";
            break;
        case Renderer::ImageComponentType::SINT:
            componentTypeName = "int";
            break;
        case Renderer::ImageComponentType::UINT:
            componentTypeName = "uint";
            break;
    }

    u8 componentCount = Renderer::ToImageComponentCount(imageDesc.format);
    if (componentCount > 1)
    {
        componentTypeName += std::to_string(componentCount);
    }

    Renderer::PixelShaderDesc pixelShaderDesc;
    pixelShaderDesc.path = "Blitting/blit.ps.hlsl";
    pixelShaderDesc.AddPermutationField("TEX_TYPE", componentTypeName);

    Renderer::GraphicsPipelineDesc pipelineDesc;
    graphResources.InitializePipelineDesc(pipelineDesc);

    pipelineDesc.states.vertexShader = renderer->LoadShader(vertexShaderDesc);
    pipelineDesc.states.pixelShader = renderer->LoadShader(pixelShaderDesc);

    pipelineDesc.renderTargets[0] = params.baseImage;

    pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;
    pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

    pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
    pipelineDesc.states.blendState.renderTargets[0].blendOp = Renderer::BlendOp::ADD;
    pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::SRC_ALPHA;
    pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::ONE;

    Renderer::GraphicsPipelineID pipeline = renderer->CreatePipeline(pipelineDesc);

    commandList.BeginPipeline(pipeline);

    _overlayDescriptorSet.Bind("_sampler", params.sampler);
    _overlayDescriptorSet.Bind("_texture", params.overlayImage);
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, &_overlayDescriptorSet, frameIndex);

    struct BlitConstant
    {
        vec4 colorMultiplier;
        vec4 additiveColor;
        u32 channelRedirectors;
    };

    BlitConstant* constants = graphResources.FrameNew<BlitConstant>();
    constants->colorMultiplier = params.colorMultiplier;
    constants->additiveColor = params.additiveColor;

    u32 channelRedirectors = params.channelRedirectors.r;
    channelRedirectors |= (params.channelRedirectors.g << 8);
    channelRedirectors |= (params.channelRedirectors.b << 16);
    channelRedirectors |= (params.channelRedirectors.a << 24);

    constants->channelRedirectors = channelRedirectors;

    commandList.PushConstant(constants, 0, sizeof(BlitConstant));

    commandList.Draw(3, 1, 0, 0);

    commandList.EndPipeline(pipeline);
    commandList.ImageBarrier(params.overlayImage);
    commandList.PopMarker();
}
