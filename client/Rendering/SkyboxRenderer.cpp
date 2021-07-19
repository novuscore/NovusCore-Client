#include "SkyboxRenderer.h"
#include "DebugRenderer.h"
#include "MapObjectRenderer.h"
#include "../Utils/ServiceLocator.h"
#include "../ECS/Components/Singletons/MapSingleton.h"

#include <Renderer/Renderer.h>
#include <Renderer/RenderGraph.h>
#include <glm/gtc/matrix_transform.hpp>
#include <tracy/TracyVulkan.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <entt.hpp>
#include <InputManager.h>
#include <GLFW/glfw3.h>

#include "CameraFreelook.h"
#include "RenderResources.h"

SkyboxRenderer::SkyboxRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer)
    : _renderer(renderer)
    , _debugRenderer(debugRenderer)
{
    CreatePermanentResources();
}

SkyboxRenderer::~SkyboxRenderer()
{

}

void SkyboxRenderer::Update(f32 deltaTime, const CameraFreeLook& camera)
{

}

void SkyboxRenderer::AddSkyboxPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex)
{
    struct SkyboxPassData
    {
        Renderer::RenderPassMutableResource color;
        Renderer::RenderPassMutableResource depth;
    };

    renderGraph->AddPass<SkyboxPassData>("Skybox Pass",
        [=](SkyboxPassData& data, Renderer::RenderGraphBuilder& builder)
        {
            data.color = builder.Write(resources.color, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD);
            data.depth = builder.Write(resources.depth, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD);

            return true; // Return true from setup to enable this pass, return false to disable it
        }, 
        [=](SkyboxPassData& data, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList)
        {
            GPU_SCOPED_PROFILER_ZONE(commandList, SkyboxPass);

            // -- Render Skybox --
            Renderer::GraphicsPipelineDesc pipelineDesc;
            graphResources.InitializePipelineDesc(pipelineDesc);

            // Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "postProcess.vs.hlsl";

            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "skybox.ps.hlsl";

            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            // Depth state
            pipelineDesc.states.depthStencilState.depthEnable = true;
            pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::EQUAL;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::NONE;
            pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

            // Render targets
            pipelineDesc.renderTargets[0] = data.color;

            pipelineDesc.depthStencil = data.depth;

            // Set pipeline
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, &resources.globalDescriptorSet, frameIndex);

            // Skyband Color Push Constant
            {
                entt::registry* registry = ServiceLocator::GetGameRegistry();
                MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

                i32 lockLight = *CVarSystem::Get()->GetIntCVar("lights.lock");
                if (!lockLight)
                {
                    _skybandColors.top = vec4(mapSingleton.GetSkybandTopColor(), 0.0f);
                    _skybandColors.middle = vec4(mapSingleton.GetSkybandMiddleColor(), 0.0f);
                    _skybandColors.bottom = vec4(mapSingleton.GetSkybandBottomColor(), 0.0f);
                    _skybandColors.aboveHorizon = vec4(mapSingleton.GetSkybandAboveHorizonColor(), 0.0f);
                    _skybandColors.horizon = vec4(mapSingleton.GetSkybandHorizonColor(), 0.0f);
                }

                commandList.PushConstant(&_skybandColors, 0, sizeof(SkybandColors));
            }

            // NumVertices hardcoded as we use a Fullscreen Triangle (Check PostProcess.vs.hlsl for more information)
            commandList.Draw(3, 1, 0, 0);

            commandList.EndPipeline(pipeline);
        });
}

void SkyboxRenderer::CreatePermanentResources()
{
    
}
