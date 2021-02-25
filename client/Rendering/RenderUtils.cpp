#include "RenderUtils.h"
#include <Renderer/Renderer.h>
#include <Renderer/RenderGraphResources.h>
#include <Renderer/CommandList.h>

#include <Renderer/Descriptors/VertexShaderDesc.h>
#include <Renderer/Descriptors/PixelShaderDesc.h>

Renderer::DescriptorSet RenderUtils::_overlayDescriptorSet;

void RenderUtils::Blit(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const BlitParams& params)
{
    commandList.PushMarker("Blit", Color::White);

    Renderer::ImageDesc imageDesc = renderer->GetImageDesc(params.input);

    // Setup pipeline
    Renderer::VertexShaderDesc vertexShaderDesc;
    vertexShaderDesc.path = "Blitting/blit.vs.hlsl";

    Renderer::ImageComponentType componentType = Renderer::ToImageComponentType(imageDesc.format);
    std::string componentTypeName = "";

    switch (componentType)
    {
        case Renderer::ImageComponentType::FLOAT:
            componentTypeName = "float";
            break;
        case Renderer::ImageComponentType::SINT:
        case Renderer::ImageComponentType::SNORM:
            componentTypeName = "int";
            break;
        case Renderer::ImageComponentType::UINT:
        case Renderer::ImageComponentType::UNORM:
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
    resources.InitializePipelineDesc(pipelineDesc);

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
    };

    BlitConstant* constants = resources.FrameNew<BlitConstant>();
    constants->colorMultiplier = vec4(1,1,1,1);
    constants->additiveColor = vec4(0,0,0,0);

    commandList.PushConstant(constants, 0, sizeof(BlitConstant));

    commandList.Draw(3, 1, 0, 0);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}

void RenderUtils::DepthBlit(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const DepthBlitParams& params)
{
    commandList.PushMarker("Blit", Color::White);

    Renderer::DepthImageDesc imageDesc = renderer->GetDepthImageDesc(params.input);

    // Setup pipeline
    Renderer::VertexShaderDesc vertexShaderDesc;
    vertexShaderDesc.path = "Blitting/blit.vs.hlsl";

    Renderer::ImageComponentType componentType = Renderer::ToImageComponentType(imageDesc.format);
    std::string componentTypeName = "";

    switch (componentType)
    {
    case Renderer::ImageComponentType::FLOAT:
        componentTypeName = "float";
        break;
    case Renderer::ImageComponentType::SINT:
    case Renderer::ImageComponentType::SNORM:
        componentTypeName = "int";
        break;
    case Renderer::ImageComponentType::UINT:
    case Renderer::ImageComponentType::UNORM:
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
    resources.InitializePipelineDesc(pipelineDesc);

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
    };

    BlitConstant* constants = resources.FrameNew<BlitConstant>();
    constants->colorMultiplier = vec4(1, 1, 1, 1);
    constants->additiveColor = vec4(0, 0, 0, 0);

    commandList.PushConstant(constants, 0, sizeof(BlitConstant));

    commandList.Draw(3, 1, 0, 0);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}

void RenderUtils::Overlay(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const OverlayParams& params)
{
    commandList.PushMarker("Overlay", Color::White);

    Renderer::ImageDesc imageDesc = renderer->GetImageDesc(params.overlayImage);

    // Setup pipeline
    Renderer::VertexShaderDesc vertexShaderDesc;
    vertexShaderDesc.path = "Blitting/blit.vs.hlsl";

    Renderer::ImageComponentType componentType = Renderer::ToImageComponentType(imageDesc.format);
    std::string componentTypeName = "";

    switch (componentType)
    {
    case Renderer::ImageComponentType::FLOAT:
        componentTypeName = "float";
        break;
    case Renderer::ImageComponentType::SINT:
    case Renderer::ImageComponentType::SNORM:
        componentTypeName = "int";
        break;
    case Renderer::ImageComponentType::UINT:
    case Renderer::ImageComponentType::UNORM:
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
    resources.InitializePipelineDesc(pipelineDesc);

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

    u32 mipLevel = params.overlayMipLevel;
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
    };

    BlitConstant* constants = resources.FrameNew<BlitConstant>();
    constants->colorMultiplier = params.overlayColorMultiplier;
    constants->additiveColor = params.overlayAdditiveColor;

    commandList.PushConstant(constants, 0, sizeof(BlitConstant));

    commandList.Draw(3, 1, 0, 0);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}

void RenderUtils::DepthOverlay(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const DepthOverlayParams& params)
{
    commandList.PushMarker("DepthOverlay", Color::White);

    // Setup pipeline
    Renderer::VertexShaderDesc vertexShaderDesc;
    vertexShaderDesc.path = "Blitting/blit.vs.hlsl";

    Renderer::DepthImageDesc imageDesc = renderer->GetDepthImageDesc(params.overlayImage);

    Renderer::ImageComponentType componentType = Renderer::ToImageComponentType(imageDesc.format);
    std::string componentTypeName = "";

    switch (componentType)
    {
        case Renderer::ImageComponentType::FLOAT:
            componentTypeName = "float";
            break;
        case Renderer::ImageComponentType::SINT:
        case Renderer::ImageComponentType::SNORM:
            componentTypeName = "int";
            break;
        case Renderer::ImageComponentType::UINT:
        case Renderer::ImageComponentType::UNORM:
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
    resources.InitializePipelineDesc(pipelineDesc);

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
    };

    BlitConstant* constants = resources.FrameNew<BlitConstant>();
    constants->colorMultiplier = params.overlayColorMultiplier;
    constants->additiveColor = params.overlayAdditiveColor;

    commandList.PushConstant(constants, 0, sizeof(BlitConstant));

    commandList.Draw(3, 1, 0, 0);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}
