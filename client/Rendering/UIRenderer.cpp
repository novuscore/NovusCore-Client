#include "UIRenderer.h"

#include <Renderer/Renderer.h>
#include <Renderer/Descriptors/FontDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/ConstantBuffer.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/UI/Singletons/UIEntityPoolSingleton.h"
#include "../ECS/Components/UI/Singletons/UIAddElementQueueSingleton.h"
#include "../ECS/Components/UI/Singletons/UIDataSingleton.h"
#include "../ECS/Components/UI/UITransform.h"
#include "../ECS/Components/UI/UITransformEvents.h"
#include "../ECS/Components/UI/UIRenderable.h"
#include "../ECS/Components/UI/UIImage.h"
#include "../ECS/Components/UI/UIText.h"
#include "../ECS/Components/UI/UIVisible.h"
#include "../ECS/Components/UI/UIVisibility.h"
#include "../ECS/Components/UI/UIDirty.h"
#include "../ECS/Components/UI/UICollidable.h"
#include "../ECS/Components/UI/UIInputField.h"
#include "../ECS/Components/UI/UICheckbox.h"

#include "../ECS/Systems/UI/UIInputSystem.h"

UIRenderer::UIRenderer(Renderer::Renderer* renderer) : _renderer(renderer)
{
    CreatePermanentResources();

    UI::InputSystem::RegisterCallbacks();

    entt::registry* registry = ServiceLocator::GetUIRegistry();
    registry->prepare<UITransform>();
    registry->prepare<UITransformEvents>();
    registry->prepare<UIRenderable>();
    registry->prepare<UIImage>();
    registry->prepare<UI::UIText>();

    registry->prepare<UIVisible>();
    registry->prepare<UIVisibility>();
    registry->prepare<UIDirty>();
    registry->prepare<UICollidable>();

    registry->prepare<UIInputField>();
    registry->prepare<UICheckbox>();

    // Register UI singletons.
    registry->set<UI::UIDataSingleton>();
    registry->set<UI::UIAddElementQueueSingleton>();

    // Register entity pool.
    registry->set<UI::UIEntityPoolSingleton>().AllocatePool();
}

void UIRenderer::AddUIPass(Renderer::RenderGraph* renderGraph, Renderer::ImageID renderTarget, u8 frameIndex)
{
    // UI Pass
    struct UIPassData
    {
        Renderer::RenderPassMutableResource renderTarget;
    };

    renderGraph->AddPass<UIPassData>("UIPass",
        [=](UIPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.renderTarget = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_LOAD);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
        [=](UIPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
        {
            TracySourceLocation(uiPass, "UIPass", tracy::Color::Yellow2);
            commandList.BeginTrace(&uiPass);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            resources.InitializePipelineDesc(pipelineDesc);

            // Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
            pipelineDesc.states.inputLayouts[0].enabled = true;
            pipelineDesc.states.inputLayouts[0].SetName("POSITION");
            pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::INPUT_FORMAT_R32G32B32_FLOAT;
            pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
            pipelineDesc.states.inputLayouts[1].enabled = true;
            pipelineDesc.states.inputLayouts[1].SetName("NORMAL");
            pipelineDesc.states.inputLayouts[1].format = Renderer::InputFormat::INPUT_FORMAT_R32G32B32_FLOAT;
            pipelineDesc.states.inputLayouts[1].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
            pipelineDesc.states.inputLayouts[2].enabled = true;
            pipelineDesc.states.inputLayouts[2].SetName("TEXCOORD");
            pipelineDesc.states.inputLayouts[2].format = Renderer::InputFormat::INPUT_FORMAT_R32G32_FLOAT;
            pipelineDesc.states.inputLayouts[2].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;

            // Render targets
            pipelineDesc.renderTargets[0] = data.renderTarget;

            // Blending
            pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
            pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::BLEND_MODE_SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::BLEND_MODE_INV_SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].srcBlendAlpha = Renderer::BlendMode::BLEND_MODE_ZERO;
            pipelineDesc.states.blendState.renderTargets[0].destBlendAlpha = Renderer::BlendMode::BLEND_MODE_ONE;

            // Panel Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/panel.vs.hlsl.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "Data/shaders/panel.ps.hlsl.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            Renderer::GraphicsPipelineID imagePipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline

            // Text Shaders
            vertexShaderDesc.path = "Data/shaders/text.vs.hlsl.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            pixelShaderDesc.path = "Data/shaders/text.ps.hlsl.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            Renderer::GraphicsPipelineID textPipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline

            // Set pipeline
            commandList.BeginPipeline(imagePipeline);
            Renderer::GraphicsPipelineID activePipeline = imagePipeline;

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            entt::registry* registry = ServiceLocator::GetUIRegistry();
            auto renderGroup = registry->group<UITransform>(entt::get<UIRenderable, UIVisible>);
            renderGroup.sort<UITransform>([](UITransform& first, UITransform& second) { return first.sortKey < second.sortKey; });
            renderGroup.each([this, &commandList, frameIndex, &registry, &activePipeline, &textPipeline, &imagePipeline](const auto entity, UITransform& transform)
                {
                    switch (transform.sortData.type)
                    {
                    case UI::UIElementType::UITYPE_TEXT:
                    case UI::UIElementType::UITYPE_INPUTFIELD:
                    {
                        UI::UIText& text = registry->get<UI::UIText>(entity);
                        if (!text.constantBuffer)
                            return;

                        if (activePipeline != textPipeline)
                        {
                            commandList.EndPipeline(activePipeline);
                            commandList.BeginPipeline(textPipeline);
                            activePipeline = textPipeline;
                        }

                        commandList.PushMarker("Text", Color(0.0f, 0.1f, 0.0f));

                        // Bind textdata descriptor
                        _drawDescriptorSet.Bind("_textData"_h, text.constantBuffer);

                        // Each glyph in the label has it's own plane and texture, this could be optimized in the future.
                        for (u32 i = 0; i < text.glyphCount; i++)
                        {
                            // Bind texture descriptor
                            _drawDescriptorSet.Bind("_texture"_h, text.textures[i]);

                            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawDescriptorSet, frameIndex);

                            // Draw
                            commandList.Draw(text.models[i]);
                        }

                        commandList.PopMarker();
                        break;
                    }
                    default:
                    {
                        UIImage& image = registry->get<UIImage>(entity);
                        if (!image.constantBuffer)
                            return;

                        if (activePipeline != imagePipeline)
                        {
                            commandList.EndPipeline(activePipeline);
                            commandList.BeginPipeline(imagePipeline);
                            activePipeline = imagePipeline;
                        }

                        commandList.PushMarker("Image", Color(0.0f, 0.1f, 0.0f));

                        // Bind descriptors
                        _drawDescriptorSet.Bind("_panelData"_h, image.constantBuffer);
                        _drawDescriptorSet.Bind("_texture"_h, image.textureID);

                        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawDescriptorSet, frameIndex);

                        // Draw
                        commandList.Draw(image.modelID);

                        commandList.PopMarker();
                        break;
                    }
                    }
                });

            commandList.EndPipeline(activePipeline);
            commandList.EndTrace();
        });
}

void UIRenderer::CreatePermanentResources()
{
    // Sampler
    Renderer::SamplerDesc samplerDesc;
    samplerDesc.enabled = true;
    samplerDesc.filter = Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.addressU = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressV = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressW = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_PIXEL;

    _linearSampler = _renderer->CreateSampler(samplerDesc);

    // Create descriptor sets
    _passDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _passDescriptorSet.Bind("_sampler"_h, _linearSampler);

    _drawDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
}
