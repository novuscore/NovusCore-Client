#include "UIRenderer.h"
#include "DebugRenderer.h"
#include <Renderer/Renderer.h>
#include <Renderer/Descriptors/FontDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Buffer.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include "CVar/CVarSystem.h"

#include "../Utils/ServiceLocator.h"

#include "../UI/ECS/Components/Singletons/UIDataSingleton.h"
#include "../UI/ECS/Components/ElementInfo.h"
#include "../UI/ECS/Components/Transform.h"
#include "../UI/ECS/Components/TransformEvents.h"
#include "../UI/ECS/Components/SortKey.h"
#include "../UI/ECS/Components/Renderable.h"
#include "../UI/ECS/Components/Image.h"
#include "../UI/ECS/Components/Text.h"
#include "../UI/ECS/Components/Visible.h"
#include "../UI/ECS/Components/Visibility.h"
#include "../UI/ECS/Components/Collision.h"
#include "../UI/ECS/Components/Collidable.h"
#include "../UI/ECS/Components/Dirty.h"
#include "../UI/ECS/Components/BoundsDirty.h"
#include "../UI/ECS/Components/InputField.h"
#include "../UI/ECS/Components/Checkbox.h"

#include "../UI/UIInputHandler.h"

AutoCVar_Int CVAR_UICollisionBoundsEnabled("ui.drawCollisionBounds", "draw collision bounds for ui elements", 0, CVarFlags::EditCheckbox);

UIRenderer::UIRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer) : _renderer(renderer), _debugRenderer(debugRenderer)
{
    CreatePermanentResources();

    UIInput::RegisterCallbacks();

    entt::registry* registry = ServiceLocator::GetUIRegistry();
    registry->prepare<UIComponent::ElementInfo>();
    registry->prepare<UIComponent::Transform>();
    registry->prepare<UIComponent::TransformEvents>();
    registry->prepare<UIComponent::SortKey>();

    registry->prepare<UIComponent::Renderable>();
    registry->prepare<UIComponent::Image>();
    registry->prepare<UIComponent::Text>();

    registry->prepare<UIComponent::Collision>();
    registry->prepare<UIComponent::Collidable>();
    registry->prepare<UIComponent::Visibility>();
    registry->prepare<UIComponent::Visible>();

    registry->prepare<UIComponent::Dirty>();
    registry->prepare<UIComponent::BoundsDirty>();

    registry->prepare<UIComponent::InputField>();
    registry->prepare<UIComponent::Checkbox>();

    // Register UI singletons.
    registry->set<UISingleton::UIDataSingleton>();

    //Reserve component space
    const int ENTITIES_TO_PREALLOCATE = 10000;
    registry->reserve(ENTITIES_TO_PREALLOCATE);

    registry->reserve<UIComponent::Transform>(ENTITIES_TO_PREALLOCATE);
    registry->reserve<UIComponent::TransformEvents>(ENTITIES_TO_PREALLOCATE);
    registry->reserve<UIComponent::SortKey>(ENTITIES_TO_PREALLOCATE);
    registry->reserve<UIComponent::Renderable>(ENTITIES_TO_PREALLOCATE);
    registry->reserve<UIComponent::Text>(ENTITIES_TO_PREALLOCATE);
    registry->reserve<UIComponent::Image>(ENTITIES_TO_PREALLOCATE);

    registry->reserve<UIComponent::Collision>(ENTITIES_TO_PREALLOCATE);
    registry->reserve<UIComponent::Collidable>(ENTITIES_TO_PREALLOCATE);

    registry->reserve<UIComponent::Visibility>(ENTITIES_TO_PREALLOCATE);
    registry->reserve<UIComponent::Visible>(ENTITIES_TO_PREALLOCATE);

    registry->reserve<UIComponent::Dirty>(ENTITIES_TO_PREALLOCATE);
    registry->reserve<UIComponent::BoundsDirty>(ENTITIES_TO_PREALLOCATE);

    registry->reserve<UIComponent::InputField>(ENTITIES_TO_PREALLOCATE);
    registry->reserve<UIComponent::Checkbox>(ENTITIES_TO_PREALLOCATE);

}

void UIRenderer::Update(f32 deltaTime)
{
    bool drawCollisionBoxes = CVAR_UICollisionBoundsEnabled.Get() == 1;
    if (drawCollisionBoxes)
    {
        const UISingleton::UIDataSingleton* dataSingleton = &ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>();

        auto collisionView = ServiceLocator::GetUIRegistry()->view<UIComponent::Collision, UIComponent::Collidable, UIComponent::Visible>();
        collisionView.each([&](UIComponent::Collision& collision) 
            {
                hvec2 min = collision.minBound / dataSingleton->UIRESOLUTION;
                hvec2 max = collision.maxBound / dataSingleton->UIRESOLUTION;

                min.y = 1.f - min.y;
                max.y = 1.f - max.y;

                uint32_t color = 0xffd9dcdf;
                _debugRenderer->DrawLine2D(min, vec2(max.x, min.y), color);
                _debugRenderer->DrawLine2D(min, vec2(min.x, max.y), color);
                _debugRenderer->DrawLine2D(vec2(min.x, max.y), max, color);
                _debugRenderer->DrawLine2D(vec2(max.x, min.y), max, color);
            });
    }
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
            
            GPU_SCOPED_PROFILER_ZONE(commandList, UIPass);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            resources.InitializePipelineDesc(pipelineDesc);

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;
            //pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

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
            auto renderGroup = registry->group<UIComponent::SortKey>(entt::get<UIComponent::Renderable, UIComponent::Visible>);
            renderGroup.sort<UIComponent::SortKey>([](UIComponent::SortKey& first, UIComponent::SortKey& second) { return first.key < second.key; });
            renderGroup.each([this, &commandList, frameIndex, &registry, &activePipeline, &textPipeline, &imagePipeline](const auto entity, UIComponent::SortKey& sortKey, UIComponent::Renderable& renderable)
            {
                switch (renderable.renderType)
                {
                case UI::RenderType::Text:
                    {
                        UIComponent::Text& text = registry->get<UIComponent::Text>(entity);
                        if (!text.constantBuffer || text.vertexBufferID == Renderer::BufferID::Invalid())
                            break;

                        if (activePipeline != textPipeline)
                        {
                            commandList.EndPipeline(activePipeline);
                            commandList.BeginPipeline(textPipeline);
                            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);
                            activePipeline = textPipeline;
                        }

                        commandList.PushMarker("Text", Color(0.0f, 0.1f, 0.0f));

                        // Bind descriptors
                        _drawTextDescriptorSet.Bind("_vertexData"_h, text.vertexBufferID);
                        _drawTextDescriptorSet.Bind("_textData"_h, text.constantBuffer->GetBuffer(frameIndex));
                        _drawTextDescriptorSet.Bind("_textureIDs"_h, text.textureIDBufferID);
                        _drawTextDescriptorSet.Bind("_textures"_h, text.font->GetTextureArray());

                        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawTextDescriptorSet, frameIndex);

                        commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);

                        commandList.DrawIndexed(6, static_cast<u32>(text.glyphCount), 0, 0, 0);

                        commandList.PopMarker();
                        break;
                    }
                case UI::RenderType::Image:
                    {
                        UIComponent::Image& image = registry->get<UIComponent::Image>(entity);
                        if (!image.constantBuffer)
                            return;

                        if (activePipeline != imagePipeline)
                        {
                            commandList.EndPipeline(activePipeline);
                            commandList.BeginPipeline(imagePipeline);
                            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);
                            activePipeline = imagePipeline;
                        }

                        commandList.PushMarker("Image", Color(0.0f, 0.1f, 0.0f));

                        // Bind descriptors
                        _drawImageDescriptorSet.Bind("_vertices"_h, image.vertexBufferID);
                        _drawImageDescriptorSet.Bind("_panelData"_h, image.constantBuffer->GetBuffer(frameIndex));
                        _drawImageDescriptorSet.Bind("_texture"_h, image.textureID);

                        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawImageDescriptorSet, frameIndex);

                        commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);

                        commandList.DrawIndexed(6, 1, 0, 0, 0);

                        commandList.PopMarker();
                        break;
                    }
                default:
                    NC_LOG_FATAL("Renderable widget tried to render with invalid render type.");
                }
            });

            commandList.EndPipeline(activePipeline);
        });
}

void UIRenderer::AddImguiPass(Renderer::RenderGraph* renderGraph, Renderer::ImageID renderTarget, u8 frameIndex)
{
    // UI Pass
    struct UIPassData
    {
        Renderer::RenderPassMutableResource renderTarget;
    };

    renderGraph->AddPass<UIPassData>("ImguiPass",
        [=](UIPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.renderTarget = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_LOAD);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
        [=](UIPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
        {
            GPU_SCOPED_PROFILER_ZONE(commandList, ImguiPass);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            resources.InitializePipelineDesc(pipelineDesc);

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;
            //pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

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

            Renderer::GraphicsPipelineID activePipeline = _renderer->CreatePipeline(pipelineDesc);

            commandList.BeginPipeline(activePipeline);
            commandList.DrawImgui();
            commandList.EndPipeline(activePipeline);
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

    // Index buffer
    static const u32 indexBufferSize = sizeof(u16) * 6;

    Renderer::BufferDesc bufferDesc;
    bufferDesc.name = "IndexBuffer";
    bufferDesc.size = indexBufferSize;
    bufferDesc.usage = Renderer::BufferUsage::BUFFER_USAGE_INDEX_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
    bufferDesc.cpuAccess = Renderer::BufferCPUAccess::None;

    _indexBuffer = _renderer->CreateBuffer(bufferDesc);

    Renderer::BufferDesc stagingBufferDesc;
    stagingBufferDesc.name = "StagingBuffer";
    stagingBufferDesc.size = indexBufferSize;
    stagingBufferDesc.usage = Renderer::BufferUsage::BUFFER_USAGE_INDEX_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
    stagingBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

    Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(stagingBufferDesc);

    u16* index = static_cast<u16*>(_renderer->MapBuffer(stagingBuffer));
    index[0] = 0;
    index[1] = 1;
    index[2] = 2;
    index[3] = 1;
    index[4] = 3;
    index[5] = 2;
    _renderer->UnmapBuffer(stagingBuffer);

    _renderer->QueueDestroyBuffer(stagingBuffer);
    _renderer->CopyBuffer(_indexBuffer, 0, stagingBuffer, 0, indexBufferSize);

    // Create descriptor sets
    _passDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _passDescriptorSet.Bind("_sampler"_h, _linearSampler);

    _drawImageDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _drawTextDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
}
