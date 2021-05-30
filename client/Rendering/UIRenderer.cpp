#include "UIRenderer.h"
#include "DebugRenderer.h"
#include "GLFW/glfw3.h"
#include "CVar/CVarSystem.h"
#include "RenderResources.h"

#include <Renderer/Renderer.h>
#include <Renderer/RenderGraph.h>
#include <Renderer/Descriptors/FontDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Buffer.h>
#include <Window/Window.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "../Utils/ServiceLocator.h"

#include "../UI/ECS/Components/Singletons/UIDataSingleton.h"
#include "../UI/ECS/Components/ElementInfo.h"
#include "../UI/ECS/Components/Name.h"
#include "../UI/ECS/Components/Relation.h"
#include "../UI/ECS/Components/Root.h"
#include "../UI/ECS/Components/SortKey.h"
#include "../UI/ECS/Components/Transform.h"
#include "../UI/ECS/Components/TransformFill.h"
#include "../UI/ECS/Components/TransformEvents.h"

#include "../UI/ECS/Components/Renderable.h"
#include "../UI/ECS/Components/Image.h"
#include "../UI/ECS/Components/ImageEventStyles.h"
#include "../UI/ECS/Components/Text.h"
#include "../UI/ECS/Components/TextEventStyles.h"

#include "../UI/ECS/Components/Visibility.h"
#include "../UI/ECS/Components/Visible.h"
#include "../UI/ECS/Components/Collision.h"
#include "../UI/ECS/Components/Collidable.h"

#include "../UI/ECS/Components/NotCulled.h"

#include "../UI/ECS/Components/Destroy.h"
#include "../UI/ECS/Components/Dirty.h"
#include "../UI/ECS/Components/BoundsDirty.h"
#include "../UI/ECS/Components/SortKeyDirty.h"

#include "../UI/ECS/Components/InputField.h"
#include "../UI/ECS/Components/Checkbox.h"
#include "../UI/ECS/Components/Slider.h"

#include "../UI/UIInputHandler.h"

AutoCVar_Int CVAR_UICollisionBoundsEnabled("ui.drawCollisionBounds", "draw collision bounds for ui elements", 0, CVarFlags::EditCheckbox);

UIRenderer::UIRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer) : _renderer(renderer), _debugRenderer(debugRenderer)
{
    CreatePermanentResources();

    entt::registry* registry = ServiceLocator::GetUIRegistry();

    // Register UI singletons.
    UISingleton::UIDataSingleton& dataSingleton = registry->set<UISingleton::UIDataSingleton>();

    // Set up UI resolution. TODO Update when window size updates.
    i32 width, height;
    glfwGetWindowSize(ServiceLocator::GetWindow()->GetWindow(), &width, &height);
    f32 aspectRatio = (static_cast<f32>(width) / static_cast<f32>(height));
    dataSingleton.UIRESOLUTION = hvec2(dataSingleton.referenceHeight * aspectRatio, dataSingleton.referenceHeight);

    //Reserve component space
    const int ENTITIES_TO_RESERVE = 10000;
    registry->reserve_pools(255);
    registry->reserve(ENTITIES_TO_RESERVE);

    registry->reserve<UIComponent::ElementInfo>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Name>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Relation>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Root>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::SortKey>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Transform>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::TransformFill>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::TransformEvents>(ENTITIES_TO_RESERVE);

    registry->reserve<UIComponent::Renderable>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Text>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::TextEventStyles>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Image>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::ImageEventStyles>(ENTITIES_TO_RESERVE);

    registry->reserve<UIComponent::Collision, UIComponent::Collidable>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Visibility, UIComponent::Visible>(ENTITIES_TO_RESERVE);

    registry->reserve<UIComponent::NotCulled>(ENTITIES_TO_RESERVE);

    registry->reserve<UIComponent::Destroy>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Dirty>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::BoundsDirty>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::SortKeyDirty>(ENTITIES_TO_RESERVE);

    registry->reserve<UIComponent::InputField>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Checkbox>(ENTITIES_TO_RESERVE);
    registry->reserve<UIComponent::Slider>(ENTITIES_TO_RESERVE);

    UIInput::RegisterCallbacks();
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

void UIRenderer::AddUIPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex)
{
    // UI Pass
    struct UIPassData
    {
        Renderer::RenderPassMutableResource color;
    };

    renderGraph->AddPass<UIPassData>("UIPass",
        [=](UIPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.color = builder.Write(resources.color, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
        [=](UIPassData& data, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList) // Execute
        {
            
            GPU_SCOPED_PROFILER_ZONE(commandList, UIPass);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            graphResources.InitializePipelineDesc(pipelineDesc);

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;
            //pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

            // Render targets
            pipelineDesc.renderTargets[0] = data.color;

            // Blending
            pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
            pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::INV_SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].srcBlendAlpha = Renderer::BlendMode::ZERO;
            pipelineDesc.states.blendState.renderTargets[0].destBlendAlpha = Renderer::BlendMode::ONE;

            // Panel Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "UI/panel.vs.hlsl";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "UI/panel.ps.hlsl";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            Renderer::GraphicsPipelineID imagePipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline

            // Text Shaders
            vertexShaderDesc.path = "UI/text.vs.hlsl";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            pixelShaderDesc.path = "UI/text.ps.hlsl";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            Renderer::GraphicsPipelineID textPipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline

            // Set pipeline
            commandList.BeginPipeline(imagePipeline);
            Renderer::GraphicsPipelineID activePipeline = imagePipeline;

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            entt::registry* registry = ServiceLocator::GetUIRegistry();
            auto renderGroup = registry->group<UIComponent::SortKey>(entt::get<UIComponent::Renderable, UIComponent::Visible, UIComponent::NotCulled>);
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

                        if (image.borderID != Renderer::TextureID::Invalid())
                        {
                            _drawImageDescriptorSet.Bind("_border"_h, image.borderID);
                        }
                        else
                        {
                            _drawImageDescriptorSet.Bind("_border"_h, _emptyBorder);
                        }

                        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawImageDescriptorSet, frameIndex);

                        commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);

                        commandList.DrawIndexed(6, 1, 0, 0, 0);

                        commandList.PopMarker();
                        break;
                    }
                default:
                    DebugHandler::PrintFatal("Renderable widget tried to render with invalid render type.");
                }
            });

            commandList.EndPipeline(activePipeline);
        });
}

void UIRenderer::AddImguiPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex)
{
    // UI Pass
    struct UIPassData
    {
        Renderer::RenderPassMutableResource color;
    };

    renderGraph->AddPass<UIPassData>("ImguiPass",
        [=](UIPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.color = builder.Write(resources.color, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
        [=](UIPassData& data, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList) // Execute
        {
            GPU_SCOPED_PROFILER_ZONE(commandList, ImguiPass);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            graphResources.InitializePipelineDesc(pipelineDesc);

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;
            //pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

            // Render targets
            pipelineDesc.renderTargets[0] = data.color;

            // Blending
            pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
            pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::INV_SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].srcBlendAlpha = Renderer::BlendMode::ZERO;
            pipelineDesc.states.blendState.renderTargets[0].destBlendAlpha = Renderer::BlendMode::ONE;

            // Panel Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "UI/panel.vs.hlsl";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "UI/panel.ps.hlsl";
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
    samplerDesc.filter = Renderer::SamplerFilter::MIN_MAG_MIP_LINEAR;
    samplerDesc.addressU = Renderer::TextureAddressMode::WRAP;
    samplerDesc.addressV = Renderer::TextureAddressMode::WRAP;
    samplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.shaderVisibility = Renderer::ShaderVisibility::PIXEL;

    _linearSampler = _renderer->CreateSampler(samplerDesc);
    _passDescriptorSet.Bind("_sampler"_h, _linearSampler);

    // Index buffer
    static const u32 indexBufferSize = sizeof(u16) * 6;

    Renderer::BufferDesc bufferDesc;
    bufferDesc.name = "IndexBuffer";
    bufferDesc.size = indexBufferSize;
    bufferDesc.usage = Renderer::BufferUsage::INDEX_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
    bufferDesc.cpuAccess = Renderer::BufferCPUAccess::None;

    _indexBuffer = _renderer->CreateBuffer(bufferDesc);

    Renderer::BufferDesc stagingBufferDesc;
    stagingBufferDesc.name = "StagingBuffer";
    stagingBufferDesc.size = indexBufferSize;
    stagingBufferDesc.usage = Renderer::BufferUsage::INDEX_BUFFER | Renderer::BufferUsage::TRANSFER_SOURCE;
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

    // Create empty border texture
    Renderer::DataTextureDesc emptyBorderDesc;
    emptyBorderDesc.debugName = "EmptyBorder";
    emptyBorderDesc.width = 1;
    emptyBorderDesc.height = 1;
    emptyBorderDesc.format = Renderer::ImageFormat::R8G8B8A8_UNORM;
    emptyBorderDesc.data = new u8[4]{ 0, 0, 0, 0 };
    
    _emptyBorder = _renderer->CreateDataTexture(emptyBorderDesc);
}
