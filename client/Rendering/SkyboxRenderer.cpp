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
    struct TerrainPassData
    {
        Renderer::RenderPassMutableResource color;
        Renderer::RenderPassMutableResource depth;
    };

    renderGraph->AddPass<TerrainPassData>("Skybox Pass", 
        [=](TerrainPassData& data, Renderer::RenderGraphBuilder& builder)
        {
            data.color = builder.Write(resources.color, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::CLEAR);
            data.depth = builder.Write(resources.depth, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::CLEAR);

            return true; // Return true from setup to enable this pass, return false to disable it
        }, 
        [=](TerrainPassData& data, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList)
        {
            GPU_SCOPED_PROFILER_ZONE(commandList, SkyboxPass);


        });
}

void SkyboxRenderer::CreatePermanentResources()
{
    
}
