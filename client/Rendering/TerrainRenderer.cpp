#include "TerrainRenderer.h"
#include "DebugRenderer.h"
#include "MapObjectRenderer.h"
#include "CModelRenderer.h"
#include "WaterRenderer.h"
#include "../Utils/ServiceLocator.h"
#include "../Utils/MapUtils.h"

#include "../ECS/Components/Singletons/MapSingleton.h"
#include "../ECS/Components/Singletons/TextureSingleton.h"
#include "../ECS/Components/Singletons/ConfigSingleton.h"

#include "CameraFreelook.h"

#include <Renderer/Renderer.h>
#include <Renderer/RenderGraph.h>
#include <glm/gtc/matrix_transform.hpp>
#include <tracy/TracyVulkan.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>
#include <entt.hpp>

#include "Camera.h"
#include "CVar/CVarSystem.h"
#include "RenderResources.h"

#define USE_PACKED_HEIGHT_RANGE 1
#define PARALLEL_LOADING 1


AutoCVar_Int CVAR_OcclusionCullingEnabled("terrain.occlusionCull.Enable", "enable culling of terrain tiles", 1, CVarFlags::EditCheckbox);
AutoCVar_Int CVAR_CullingEnabled("terrain.culling.Enable", "enable culling of terrain tiles", 1, CVarFlags::EditCheckbox);
AutoCVar_Int CVAR_GPUCullingEnabled("terrain.culling.GPUCullEnable", "enable gpu culling", 1, CVarFlags::EditCheckbox);
AutoCVar_Int CVAR_LockCullingFrustum("terrain.culling.LockFrustum", "lock frustrum for terrain culling", 0, CVarFlags::EditCheckbox);

AutoCVar_Int CVAR_HeightBoxEnable("terrain.heightBox.Enable", "draw height box", 1, CVarFlags::EditCheckbox);
AutoCVar_Float CVAR_HeightBoxScale("terrain.heightBox.Scale", "size of the height box", 0.1f, CVarFlags::EditFloatDrag);
AutoCVar_VecFloat CVAR_HeightBoxPosition("terrain.heightBox.Position", "position of the height box", vec4(0, 0, 0, 0), CVarFlags::Noedit);
AutoCVar_Int CVAR_HeightBoxLockPosition("terrain.heightBox.LockPosition", "lock height box position", 0, CVarFlags::EditCheckbox);

AutoCVar_Int CVAR_DrawCellGrid("terrain.cellGrid.Enable", "draw debug grid for displaying cells", 1, CVarFlags::EditCheckbox);

struct TerrainChunkData
{
    u32 alphaMapID = 0;
};

struct TerrainCellData
{
    u16 diffuseIDs[4] = { 0, 0, 0, 0 };
    u16 hole;
    u16 _padding;
};

struct TerrainCellHeightRange
{
#if USE_PACKED_HEIGHT_RANGE
    u32 minmax;
#else
    float min;
    float max;
#endif
};

TerrainRenderer::TerrainRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer, CModelRenderer* complexModelRenderer)
    : _renderer(renderer)
    , _debugRenderer(debugRenderer)
    , _complexModelRenderer(complexModelRenderer)
{
    _mapObjectRenderer = new MapObjectRenderer(renderer, debugRenderer); // Needs to be created before CreatePermanentResources
    _waterRenderer = new WaterRenderer(renderer); // Needs to be created before CreatePermanentResources
    CreatePermanentResources();
}

TerrainRenderer::~TerrainRenderer()
{
    delete _mapObjectRenderer;
}

void TerrainRenderer::Update(f32 deltaTime)
{
    //for (const Terrain::MapUtils::AABoundingBox& boundingBox : _cellBoundingBoxes)
    //{
    //    _debugRenderer->DrawAABB3D(boundingBox.min, boundingBox.max, 0xff00ff00);
    //}

    Camera* camera = ServiceLocator::GetCamera();

    if (CVAR_HeightBoxEnable.Get())
    {
        if (!CVAR_HeightBoxLockPosition.Get())
        {
            vec4 position = vec4(camera->GetPosition(), 0);
            position.z = Terrain::MapUtils::GetHeightFromWorldPosition(position);

            CVAR_HeightBoxPosition.Set(position);
        }

        f32 halfSize = CVAR_HeightBoxScale.GetFloat();
        vec3 min = CVAR_HeightBoxPosition.Get();
        min.x -= halfSize;
        min.y -= halfSize;

        vec3 max = CVAR_HeightBoxPosition.Get();
        max.x += halfSize;
        max.y += halfSize;

        max.z += halfSize;

        _debugRenderer->DrawAABB3D(min, max, 0xff00ff00);
    }
    
    if (CVAR_DrawCellGrid.Get())
    {
        DebugRenderCellTriangles(camera);
    }

    const bool cullingEnabled = CVAR_CullingEnabled.Get();
    const bool gpuCullEnabled = CVAR_GPUCullingEnabled.Get();

    if (cullingEnabled && !gpuCullEnabled)
    {
        CPUCulling(camera);
    }

    // Read back from culling counters
    u32 numDrawCalls = Terrain::MAP_CELLS_PER_CHUNK * static_cast<u32>(_loadedChunks.Size());
    _numSurvivingDrawCalls = numDrawCalls;

    if (cullingEnabled)
    {
        if (gpuCullEnabled)
        {
            u32* count = static_cast<u32*>(_renderer->MapBuffer(_drawCountReadBackBuffer));
            if (count != nullptr)
            {
                _numSurvivingDrawCalls = *count;
            }
            _renderer->UnmapBuffer(_drawCountReadBackBuffer);
        }
        else
        {
            _numSurvivingDrawCalls = static_cast<u32>(_culledInstances.size());
        }
    }

    // Subrenderers
    _mapObjectRenderer->Update(deltaTime);
    _waterRenderer->Update(deltaTime);
}

__forceinline bool IsInsideFrustum(const vec4* planes, const Geometry::AABoundingBox& boundingBox)
{
    // this is why god abandoned us
    for (int i = 0; i < 6; ++i)
    {
        const vec4& plane = planes[i];

        vec3 vmin, vmax;

        // X axis 
        if (plane.x > 0) {
            vmin.x = boundingBox.min.x;
            vmax.x = boundingBox.max.x;
        }
        else {
            vmin.x = boundingBox.max.x;
            vmax.x = boundingBox.min.x;
        }
        // Y axis 
        if (plane.y > 0) {
            vmin.y = boundingBox.min.y;
            vmax.y = boundingBox.max.y;
        }
        else {
            vmin.y = boundingBox.max.y;
            vmax.y = boundingBox.min.y;
        }
        // Z axis 
        if (plane.z > 0)
        {
            vmin.z = boundingBox.min.z;
            vmax.z = boundingBox.max.z;
        }
        else
        {
            vmin.z = boundingBox.max.z;
            vmax.z = boundingBox.min.z;
        }

        if (glm::dot(vec3(plane), vmin) + plane.w < 0)
        {
            return false;
        }
    }

    return true;
}

void TerrainRenderer::CPUCulling(const Camera* camera)
{
    ZoneScoped;

    static vec4 frustumPlanes[6];
    static mat4x4 lockedViewProjectionMatrix;

    if (!CVAR_LockCullingFrustum.Get())
    {
        memcpy(frustumPlanes, camera->GetFrustumPlanes(), sizeof(frustumPlanes));
        lockedViewProjectionMatrix = camera->GetViewProjectionMatrix();
    }

    _culledInstances.clear();
    _culledInstances.reserve(_loadedChunks.Size() * Terrain::MAP_CELLS_PER_CHUNK);

    const size_t chunkCount = _loadedChunks.Size();
    size_t boundingBoxIndex = 0;
    for (size_t i = 0; i < chunkCount; ++i)
    {
        for (u16 cellId = 0; cellId < Terrain::MAP_CELLS_PER_CHUNK; ++cellId)
        {
            u32 index = static_cast<u32>(boundingBoxIndex++);

            const Geometry::AABoundingBox* boundingBox = nullptr;
            _cellBoundingBoxes.ReadLock([&](const std::vector<Geometry::AABoundingBox>& boundingBoxes) {
                boundingBox = &boundingBoxes[index];
                });

            //const Geometry::AABoundingBox& boundingBox = _cellBoundingBoxes[index];
            if (IsInsideFrustum(frustumPlanes, *boundingBox))
            {
                u16 chunkId = 0;
                _loadedChunks.ReadLock(
                    [&](const std::vector<u16>& loadedChunks)
                    {
                        chunkId = loadedChunks[i];
                    });

                CellInstance& cellInstance = _culledInstances.emplace_back();
                cellInstance.packedChunkCellID = (chunkId << 16) | cellId;
                cellInstance.instanceID = index;
            }
        }
    }

    _debugRenderer->DrawFrustum(lockedViewProjectionMatrix, 0xff0000ff);
}

void TerrainRenderer::DebugRenderCellTriangles(const Camera* camera)
{
    std::vector<Geometry::Triangle> triangles = Terrain::MapUtils::GetCellTrianglesFromWorldPosition(camera->GetPosition());
    {
        for (auto& triangle : triangles)
        {
            f32 steepnessAngle = triangle.GetSteepnessAngle();
            u32 color = steepnessAngle <= 50 ? 0xff00ff00 : 0xff0000ff;
            // Offset Y slightly to not be directly drawn on top of the terrain
            triangle.vert1.z += 0.1f;
            triangle.vert2.z += 0.1f;
            triangle.vert3.z += 0.1f;

            _debugRenderer->DrawLine3D(triangle.vert1, triangle.vert2, color);
            _debugRenderer->DrawLine3D(triangle.vert2, triangle.vert3, color);
            _debugRenderer->DrawLine3D(triangle.vert3, triangle.vert1, color);
        }
    }
}

void TerrainRenderer::AddTerrainDepthPrepass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex)
{
    // Terrain Depth Prepass
    {
        struct TerrainDepthPrepassData
        {
            Renderer::RenderPassMutableResource depth;
        };

        const bool cullingEnabled = CVAR_CullingEnabled.Get();
        const bool gpuCullEnabled = CVAR_GPUCullingEnabled.Get();
        const bool lockFrustum = CVAR_LockCullingFrustum.Get();

        renderGraph->AddPass<TerrainDepthPrepassData>("Terrain Depth Prepass",
            [=](TerrainDepthPrepassData& data, Renderer::RenderGraphBuilder& builder) // Setup
            {
                data.depth = builder.Write(resources.depth, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::CLEAR);

                return true; // Return true from setup to enable this pass, return false to disable it
            },
            [=](TerrainDepthPrepassData& data, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList) // Execute
            {
                entt::registry* registry = ServiceLocator::GetGameRegistry();
                MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

                Terrain::Map& currentMap = mapSingleton.GetCurrentMap();

                if (!currentMap.IsLoadedMap())
                    return;

                if (currentMap.header.flags.UseMapObjectInsteadOfTerrain)
                    return;

                GPU_SCOPED_PROFILER_ZONE(commandList, TerrainDepthPrepass);

                // Upload culled instances
                if (cullingEnabled && !gpuCullEnabled && !_culledInstances.empty())
                {
                    size_t size = sizeof(CellInstance) * _culledInstances.size();
                    auto uploadBuffer = _renderer->CreateUploadBuffer(_culledInstanceBuffer, 0, size);
                    memcpy(uploadBuffer->mappedMemory, _culledInstances.data(), size);

                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToIndirectArguments, _culledInstanceBuffer);
                }

                // Cull instances on GPU
                if (cullingEnabled && gpuCullEnabled)
                {
                    Renderer::ComputePipelineDesc pipelineDesc;
                    graphResources.InitializePipelineDesc(pipelineDesc);

                    Renderer::ComputeShaderDesc shaderDesc;
                    shaderDesc.path = "terrainCulling.cs.hlsl";
                    pipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);

                    Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(pipelineDesc);
                    commandList.BeginPipeline(pipeline);

                    if (!lockFrustum)
                    {
                        Camera* camera = ServiceLocator::GetCamera();
                        memcpy(_cullingConstants.frustumPlanes, camera->GetFrustumPlanes(), sizeof(_cullingConstants.frustumPlanes));
                    }
                    _cullingConstants.occlusionEnabled = CVAR_OcclusionCullingEnabled.Get();

                    // Reset the counter
                    commandList.FillBuffer(_argumentBuffer, 0, 4, Terrain::NUM_INDICES_PER_CELL);
                    commandList.FillBuffer(_argumentBuffer, 4, 12, 0);
                    commandList.FillBuffer(_argumentBuffer, 16, 4, 0);
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, _argumentBuffer);

                    commandList.PushConstant(&_cullingConstants, 0, sizeof(CullingConstants));

                    _cullingPassDescriptorSet.Bind("_instances", _instanceBuffer);
                    _cullingPassDescriptorSet.Bind("_heightRanges", _cellHeightRangeBuffer);
                    _cullingPassDescriptorSet.Bind("_culledInstances", _culledInstanceBuffer);
                    _cullingPassDescriptorSet.Bind("_drawCount", _argumentBuffer);

                    Renderer::SamplerDesc samplerDesc;
                    samplerDesc.filter = Renderer::SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR;

                    samplerDesc.addressU = Renderer::TextureAddressMode::CLAMP;
                    samplerDesc.addressV = Renderer::TextureAddressMode::CLAMP;
                    samplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
                    samplerDesc.minLOD = 0.f;
                    samplerDesc.maxLOD = 16.f;
                    samplerDesc.mode = Renderer::SamplerReductionMode::MIN;

                    Renderer::SamplerID occlusionSampler = _renderer->CreateSampler(samplerDesc);

                    _cullingPassDescriptorSet.Bind("_depthSampler", occlusionSampler);
                    _cullingPassDescriptorSet.Bind("_depthPyramid", resources.depthPyramid);

                    // Bind descriptorset
                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::DEBUG, &resources.debugDescriptorSet, frameIndex);
                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, &resources.globalDescriptorSet, frameIndex);
                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_cullingPassDescriptorSet, frameIndex);

                    const u32 cellCount = (u32)_loadedChunks.Size() * Terrain::MAP_CELLS_PER_CHUNK;
                    commandList.Dispatch((cellCount + 31) / 32, 1, 1);

                    commandList.EndPipeline(pipeline);

                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToVertexBuffer, _culledInstanceBuffer);
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _argumentBuffer);
                }

                Renderer::GraphicsPipelineDesc pipelineDesc;
                graphResources.InitializePipelineDesc(pipelineDesc);

                // Shaders
                Renderer::VertexShaderDesc vertexShaderDesc;
                vertexShaderDesc.path = "terrain.vs.hlsl";
                vertexShaderDesc.AddPermutationField("COLOR_PASS", "0");

                pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

                // Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
                pipelineDesc.states.inputLayouts[0].enabled = true;
                pipelineDesc.states.inputLayouts[0].SetName("TEXCOORD0");
                pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::R32_UINT;
                pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::PER_INSTANCE;
                pipelineDesc.states.inputLayouts[1].enabled = true;
                pipelineDesc.states.inputLayouts[1].SetName("TEXCOORD1");
                pipelineDesc.states.inputLayouts[1].format = Renderer::InputFormat::R32_UINT;
                pipelineDesc.states.inputLayouts[1].inputClassification = Renderer::InputClassification::PER_INSTANCE;

                // Depth state
                pipelineDesc.states.depthStencilState.depthEnable = true;
                pipelineDesc.states.depthStencilState.depthWriteEnable = true;
                pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::GREATER;

                // Rasterizer state
                pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;
                pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

                // Render targets
                pipelineDesc.depthStencil = data.depth;

                // Set pipeline
                Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
                commandList.BeginPipeline(pipeline);

                // Set instance buffer
                const Renderer::BufferID instanceBuffer = cullingEnabled ? _culledInstanceBuffer : _instanceBuffer;
                commandList.SetBuffer(0, instanceBuffer);

                // Set index buffer
                commandList.SetIndexBuffer(_cellIndexBuffer, Renderer::IndexFormat::UInt16);

                // Bind viewbuffer
                _passDescriptorSet.Bind("_packedVertices"_h, _vertexBuffer);
                _passDescriptorSet.Bind("_packedCellData"_h, _cellBuffer);

                // Bind descriptorset
                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, &resources.globalDescriptorSet, frameIndex);
                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);
                if (cullingEnabled)
                {
                    if (gpuCullEnabled)
                    {
                        commandList.DrawIndexedIndirect(_argumentBuffer, 0, 1);
                    }
                    else
                    {
                        const u32 cellCount = (u32)_culledInstances.size();
                        TracyPlot("Cell Instance Count", (i64)cellCount);
                        commandList.DrawIndexed(Terrain::NUM_INDICES_PER_CELL, cellCount, 0, 0, 0);
                    }
                }
                else
                {
                    const u32 cellCount = Terrain::MAP_CELLS_PER_CHUNK * (u32)_loadedChunks.Size();
                    TracyPlot("Cell Instance Count", (i64)cellCount);
                    commandList.DrawIndexed(Terrain::NUM_INDICES_PER_CELL, cellCount, 0, 0, 0);
                }

                commandList.EndPipeline(pipeline);
                if (cullingEnabled)
                {
                    if (gpuCullEnabled)
                    {
                        commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToTransferSrc, _argumentBuffer);
                        commandList.CopyBuffer(_drawCountReadBackBuffer, 0, _argumentBuffer, 4, 4);
                        commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToTransferSrc, _drawCountReadBackBuffer);
                    }
                }
            });
    }

    _mapObjectRenderer->AddMapObjectDepthPrepass(renderGraph, resources, frameIndex);
}

void TerrainRenderer::AddTerrainPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex)
{
    // Terrain Pass
    {
        struct TerrainPassData
        {
            Renderer::RenderPassMutableResource color;
            Renderer::RenderPassMutableResource objectIDs;
            Renderer::RenderPassMutableResource depth;
        };

        const bool cullingEnabled = CVAR_CullingEnabled.Get();
        const bool gpuCullEnabled = CVAR_GPUCullingEnabled.Get();
        const bool lockFrustum = CVAR_LockCullingFrustum.Get();

        renderGraph->AddPass<TerrainPassData>("Terrain Pass",
            [=](TerrainPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.color = builder.Write(resources.color, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::CLEAR);
            data.objectIDs = builder.Write(resources.objectIDs, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::CLEAR);
            data.depth = builder.Write(resources.depth, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::CLEAR);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
            [=](TerrainPassData& data, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList) // Execute
        {
            entt::registry* registry = ServiceLocator::GetGameRegistry();
            MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

            Terrain::Map& currentMap = mapSingleton.GetCurrentMap();

            if (!currentMap.IsLoadedMap())
                return;

            if (currentMap.header.flags.UseMapObjectInsteadOfTerrain)
                return;

            GPU_SCOPED_PROFILER_ZONE(commandList, TerrainPass);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            graphResources.InitializePipelineDesc(pipelineDesc);

            // Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "terrain.vs.hlsl";
            vertexShaderDesc.AddPermutationField("COLOR_PASS", "1");

            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "terrain.ps.hlsl";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            // Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
            pipelineDesc.states.inputLayouts[0].enabled = true;
            pipelineDesc.states.inputLayouts[0].SetName("TEXCOORD0");
            pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::R32_UINT;
            pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::PER_INSTANCE;
            pipelineDesc.states.inputLayouts[1].enabled = true;
            pipelineDesc.states.inputLayouts[1].SetName("TEXCOORD1");
            pipelineDesc.states.inputLayouts[1].format = Renderer::InputFormat::R32_UINT;
            pipelineDesc.states.inputLayouts[1].inputClassification = Renderer::InputClassification::PER_INSTANCE;

            // Depth state
            pipelineDesc.states.depthStencilState.depthEnable = true;
            pipelineDesc.states.depthStencilState.depthWriteEnable = false;
            pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::EQUAL;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;
            pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

            // Render targets
            pipelineDesc.renderTargets[0] = data.color;
            pipelineDesc.renderTargets[1] = data.objectIDs;

            pipelineDesc.depthStencil = data.depth;

            // Set pipeline
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            // Set instance buffer
            const Renderer::BufferID instanceBuffer = cullingEnabled ? _culledInstanceBuffer : _instanceBuffer;
            commandList.SetBuffer(0, instanceBuffer);

            // Set index buffer
            commandList.SetIndexBuffer(_cellIndexBuffer, Renderer::IndexFormat::UInt16);

            // Bind viewbuffer
            _passDescriptorSet.Bind("_packedVertices"_h, _vertexBuffer);
            _passDescriptorSet.Bind("_packedCellData"_h, _cellBuffer);
            _passDescriptorSet.Bind("_chunkData"_h, _chunkBuffer);
            _passDescriptorSet.Bind("_ambientOcclusion", resources.ambientObscurance);

            // Bind descriptorset
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, &resources.globalDescriptorSet, frameIndex);
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);
            if (cullingEnabled)
            {
                if (gpuCullEnabled)
                {
                    commandList.DrawIndexedIndirect(_argumentBuffer, 0, 1);
                }
                else
                {
                    const u32 cellCount = (u32)_culledInstances.size();
                    TracyPlot("Cell Instance Count", (i64)cellCount);
                    commandList.DrawIndexed(Terrain::NUM_INDICES_PER_CELL, cellCount, 0, 0, 0);
                }
            }
            else
            {
                const u32 cellCount = Terrain::MAP_CELLS_PER_CHUNK * (u32)_loadedChunks.Size();
                TracyPlot("Cell Instance Count", (i64)cellCount);
                commandList.DrawIndexed(Terrain::NUM_INDICES_PER_CELL, cellCount, 0, 0, 0);
            }

            commandList.EndPipeline(pipeline);
        });
    }

    // Subrenderers
    _mapObjectRenderer->AddMapObjectPass(renderGraph, resources, frameIndex); 
   // _waterRenderer->AddWaterPass(renderGraph, globalDescriptorSet, colorTarget, depthTarget, frameIndex);
}

void TerrainRenderer::CreatePermanentResources()
{
    entt::registry* registry = ServiceLocator::GetGameRegistry();
    MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

    // Create texture array
    Renderer::TextureArrayDesc textureColorArrayDesc;
    textureColorArrayDesc.size = 4096;

    _terrainColorTextureArray = _renderer->CreateTextureArray(textureColorArrayDesc);
    _passDescriptorSet.Bind("_terrainColorTextures"_h, _terrainColorTextureArray);

    Renderer::TextureArrayDesc textureAlphaArrayDesc;
    textureAlphaArrayDesc.size = Terrain::MAP_CHUNKS_PER_MAP;

    _terrainAlphaTextureArray = _renderer->CreateTextureArray(textureAlphaArrayDesc);
    _passDescriptorSet.Bind("_terrainAlphaTextures"_h, _terrainAlphaTextureArray);

    // Create and load a 1x1 pixel RGBA8 unorm texture with zero'ed data so we can use textureArray[0] as "invalid" textures, sampling it will return 0.0f on all channels
    Renderer::DataTextureDesc zeroColorTexture;
    zeroColorTexture.debugName = "TerrainZeroColor";
    zeroColorTexture.layers = 1;
    zeroColorTexture.width = 1;
    zeroColorTexture.height = 1;
    zeroColorTexture.format = Renderer::ImageFormat::R8G8B8A8_UNORM;
    zeroColorTexture.data = new u8[4]{ 0, 0, 0, 0 };

    u32 index;
    _renderer->CreateDataTextureIntoArray(zeroColorTexture, _terrainColorTextureArray, index);

    delete[] zeroColorTexture.data;

    // Samplers
    Renderer::SamplerDesc alphaSamplerDesc;
    alphaSamplerDesc.enabled = true;
    alphaSamplerDesc.filter = Renderer::SamplerFilter::MIN_MAG_MIP_LINEAR;
    alphaSamplerDesc.addressU = Renderer::TextureAddressMode::CLAMP;
    alphaSamplerDesc.addressV = Renderer::TextureAddressMode::CLAMP;
    alphaSamplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
    alphaSamplerDesc.shaderVisibility = Renderer::ShaderVisibility::PIXEL;

    _alphaSampler = _renderer->CreateSampler(alphaSamplerDesc);
    _passDescriptorSet.Bind("_alphaSampler"_h, _alphaSampler);

    Renderer::SamplerDesc colorSamplerDesc;
    colorSamplerDesc.enabled = true;
    colorSamplerDesc.filter = Renderer::SamplerFilter::MIN_MAG_MIP_LINEAR;
    colorSamplerDesc.addressU = Renderer::TextureAddressMode::WRAP;
    colorSamplerDesc.addressV = Renderer::TextureAddressMode::WRAP;
    colorSamplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
    colorSamplerDesc.shaderVisibility = Renderer::ShaderVisibility::PIXEL;

    _colorSampler = _renderer->CreateSampler(colorSamplerDesc);
    _passDescriptorSet.Bind("_colorSampler"_h, _colorSampler);

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainCellIndexBuffer";
        desc.size = Terrain::NUM_INDICES_PER_CELL * sizeof(u16);
        desc.usage = Renderer::BufferUsage::INDEX_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        _cellIndexBuffer = _renderer->CreateBuffer(_cellIndexBuffer, desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainArgumentBuffer";
        desc.size = sizeof(VkDrawIndexedIndirectCommand);
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::INDIRECT_ARGUMENT_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION | Renderer::BufferUsage::TRANSFER_SOURCE;
        _argumentBuffer = _renderer->CreateBuffer(_argumentBuffer, desc);

        desc.size = sizeof(u32);
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::ReadOnly;
        _drawCountReadBackBuffer = _renderer->CreateBuffer(_drawCountReadBackBuffer, desc);
    }

    // Upload cell index buffer
    {
        Renderer::BufferDesc indexUploadBufferDesc;
        indexUploadBufferDesc.name = "TerrainCellIndexUploadBuffer";
        indexUploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
        indexUploadBufferDesc.size = sizeof(u16) * Terrain::NUM_INDICES_PER_CELL;
        indexUploadBufferDesc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;

        Renderer::BufferID indexUploadBuffer = _renderer->CreateBuffer(indexUploadBufferDesc);
        _renderer->QueueDestroyBuffer(indexUploadBuffer);

        u16* indices = static_cast<u16*>(_renderer->MapBuffer(indexUploadBuffer));

        // Fill index buffer
        size_t indexIndex = 0;
        for (u16 row = 0; row < Terrain::MAP_CELL_INNER_GRID_STRIDE; row++)
        {
            for (u16 col = 0; col < Terrain::MAP_CELL_INNER_GRID_STRIDE; col++)
            {
                const u16 baseVertex = (row * Terrain::MAP_CELL_TOTAL_GRID_STRIDE + col);

                //1     2
                //   0
                //3     4

                const u16 topLeftVertex = baseVertex;
                const u16 topRightVertex = baseVertex + 1;
                const u16 bottomLeftVertex = baseVertex + Terrain::MAP_CELL_TOTAL_GRID_STRIDE;
                const u16 bottomRightVertex = baseVertex + Terrain::MAP_CELL_TOTAL_GRID_STRIDE + 1;
                const u16 centerVertex = baseVertex + Terrain::MAP_CELL_OUTER_GRID_STRIDE;

                // Up triangle
                indices[indexIndex++] = centerVertex;
                indices[indexIndex++] = topRightVertex;
                indices[indexIndex++] = topLeftVertex;

                // Left triangle
                indices[indexIndex++] = centerVertex;
                indices[indexIndex++] = topLeftVertex;
                indices[indexIndex++] = bottomLeftVertex;

                // Down triangle
                indices[indexIndex++] = centerVertex;
                indices[indexIndex++] = bottomLeftVertex;
                indices[indexIndex++] = bottomRightVertex;

                // Right triangle
                indices[indexIndex++] = centerVertex;
                indices[indexIndex++] = bottomRightVertex;
                indices[indexIndex++] = topRightVertex;
            }
        }

        _renderer->UnmapBuffer(indexUploadBuffer);
        _renderer->CopyBuffer(_cellIndexBuffer, 0, indexUploadBuffer, 0, indexUploadBufferDesc.size);
    }

    // Check if we should load a default map specified by Config
    {
        ConfigSingleton& configSingleton = registry->ctx<ConfigSingleton>();
        const std::string& defaultMap = configSingleton.uiConfig.GetDefaultMap();

        if (defaultMap.length() != 0)
        {
            u32 defaultMapHash = StringUtils::fnv1a_32(defaultMap.c_str(), defaultMap.length());
            const NDBC::Map* defaultMap = mapSingleton.GetMapByNameHash(defaultMapHash);
            if (defaultMap != nullptr)
            {
                CameraFreeLook* cameraFreeLook = ServiceLocator::GetCameraFreeLook();
                cameraFreeLook->LoadFromFile("freelook.cameradata");
                LoadMap(defaultMap);
            }
        }
    }
}

void TerrainRenderer::RegisterChunksToBeLoaded(Terrain::Map& map, ivec2 middleChunk, u16 drawDistance)
{
    // Middle position has to be within map grid
    assert(middleChunk.x >= 0);
    assert(middleChunk.y >= 0);
    assert(middleChunk.x < 64);
    assert(middleChunk.y < 64);

    assert(drawDistance > 0);
    assert(drawDistance <= 64);

    u16 radius = drawDistance - 1;

    ivec2 startPos = ivec2(middleChunk.x - radius, middleChunk.y - radius);
    startPos = glm::max(startPos, ivec2(0, 0));

    ivec2 endPos = ivec2(middleChunk.x + radius, middleChunk.y + radius);
    endPos = glm::min(endPos, ivec2(63, 63));

    for (i32 y = startPos.y; y <= endPos.y; y++)
    {
        for (i32 x = startPos.x; x <= endPos.x; x++)
        {
            RegisterChunkToBeLoaded(map, x, y);
        }
    }
}

void TerrainRenderer::RegisterChunkToBeLoaded(Terrain::Map& map, u16 chunkPosX, u16 chunkPosY)
{
    u16 chunkID;
    map.GetChunkIdFromChunkPosition(chunkPosX, chunkPosY, chunkID);

    const auto chunkIt = map.chunks.find(chunkID);
    if (chunkIt == map.chunks.cend())
    {
        return;
    }

    ChunkToBeLoaded& chunkToBeLoaded = _chunksToBeLoaded.emplace_back();
    chunkToBeLoaded.map = &map;
    chunkToBeLoaded.chunk = &chunkIt->second;
    chunkToBeLoaded.chunkPosX = chunkPosX;
    chunkToBeLoaded.chunkPosY = chunkPosY;
    chunkToBeLoaded.chunkID = chunkID;
}

void TerrainRenderer::ExecuteLoad()
{
    ZoneScopedN("TerrainRenderer::ExecuteLoad()");

    size_t numChunksToLoad = _chunksToBeLoaded.size();

    {
        Renderer::BufferDesc desc;
        desc.name = "CulledTerrainInstanceBuffer";
        desc.size = sizeof(CellInstance) * Terrain::MAP_CELLS_PER_CHUNK * numChunksToLoad;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::VERTEX_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        _instanceBuffer = _renderer->CreateBuffer(_instanceBuffer, desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainInstanceBuffer";
        desc.size = sizeof(CellInstance) * Terrain::MAP_CELLS_PER_CHUNK * numChunksToLoad;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::VERTEX_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        _culledInstanceBuffer = _renderer->CreateBuffer(_culledInstanceBuffer, desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainChunkBuffer";
        desc.size = sizeof(TerrainChunkData) * numChunksToLoad;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        _chunkBuffer = _renderer->CreateBuffer(_chunkBuffer, desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainCellBuffer";
        desc.size = sizeof(TerrainCellData) * Terrain::MAP_CELLS_PER_CHUNK * numChunksToLoad;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        _cellBuffer = _renderer->CreateBuffer(_cellBuffer, desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainVertexBuffer";
        desc.size = sizeof(TerrainVertex) * Terrain::NUM_VERTICES_PER_CHUNK * numChunksToLoad;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        _vertexBuffer = _renderer->CreateBuffer(_vertexBuffer, desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "CellHeightRangeBuffer";
        desc.size = sizeof(TerrainCellHeightRange) * Terrain::MAP_CELLS_PER_CHUNK * numChunksToLoad;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        _cellHeightRangeBuffer = _renderer->CreateBuffer(_cellHeightRangeBuffer, desc);
    }

#if PARALLEL_LOADING
    tf::Taskflow tf;
    tf.parallel_for(_chunksToBeLoaded.begin(), _chunksToBeLoaded.end(), [&](const auto& chunk)
        {
            std::string chunkIDString = std::to_string(chunk.chunkID);

            ZoneScoped;
            ZoneText(chunkIDString.c_str(), chunkIDString.length());

            LoadChunk(chunk);
        });
    tf.wait_for_all();
#else
    for (const ChunkToBeLoaded& chunk : _chunksToBeLoaded)
    {
        std::string chunkIDString = std::to_string(chunk.chunkID);

        ZoneScoped;
        ZoneText(chunkIDString.c_str(), chunkIDString.length());

        LoadChunk(chunk);
    }
#endif

    _chunksToBeLoaded.clear();
}

bool TerrainRenderer::LoadMap(const NDBC::Map* map)
{
    entt::registry* registry = ServiceLocator::GetGameRegistry();
    MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

    if (!Terrain::MapUtils::LoadMap(registry, map))
        return false;

    Terrain::Map& currentMap = mapSingleton.GetCurrentMap();

    // Clear Terrain, WMOs and Water
    _loadedChunks.Clear();
    _cellBoundingBoxes.Clear();
    _mapObjectRenderer->Clear();
    _complexModelRenderer->Clear();
    _waterRenderer->Clear();

    // Unload everything but the first texture in our color array
    _renderer->UnloadTexturesInArray(_terrainColorTextureArray, 1);
    // Unload everything in our alpha array
    _renderer->UnloadTexturesInArray(_terrainAlphaTextureArray, 0);

    // Register Map Object to be loaded
    if (currentMap.header.flags.UseMapObjectInsteadOfTerrain)
    {
        _mapObjectRenderer->RegisterMapObjectToBeLoaded(currentMap.header.mapObjectName, currentMap.header.mapObjectPlacement);
    }
    else
    {
        RegisterChunksToBeLoaded(currentMap, ivec2(32, 32), 32); // Load everything
        //RegisterChunksToBeLoaded(currentMap, ivec2(31, 49), 1); // bugged terrain
        //RegisterChunksToBeLoaded(mapSingleton.currentMap, ivec2(31, 49), 1); // Goldshire
        //RegisterChunksToBeLoaded(map, ivec2(40, 32), 8); // Razor Hill
        //RegisterChunksToBeLoaded(map, ivec2(22, 25), 8); // Borean Tundra

        ExecuteLoad();
    
        // Upload instance data
        {
            const size_t cellCount = Terrain::MAP_CELLS_PER_CHUNK * _loadedChunks.Size();

            Renderer::BufferDesc uploadBufferDesc;
            uploadBufferDesc.name = "TerrainInstanceUploadBuffer";
            uploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
            uploadBufferDesc.size = sizeof(CellInstance) * cellCount;
            uploadBufferDesc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;

            Renderer::BufferID instanceUploadBuffer = _renderer->CreateBuffer(uploadBufferDesc);
            _renderer->QueueDestroyBuffer(instanceUploadBuffer);

            void* instanceBufferMemory = _renderer->MapBuffer(instanceUploadBuffer);
            CellInstance* instanceData = static_cast<CellInstance*>(instanceBufferMemory);
            u32 instanceDataIndex = 0;

            _loadedChunks.ReadLock(
                [&](std::vector<u16> loadedChunks)
                {
                    for (const u16 chunkID : loadedChunks)
                    {
                        for (u32 cellID = 0; cellID < Terrain::MAP_CELLS_PER_CHUNK; ++cellID)
                        {
                            instanceData[instanceDataIndex].packedChunkCellID = (chunkID << 16) | (cellID & 0xffff);
                            instanceData[instanceDataIndex++].instanceID = instanceDataIndex;
                        }
                    }
                });
            
            assert(instanceDataIndex == cellCount);
            _renderer->UnmapBuffer(instanceUploadBuffer);
            _renderer->CopyBuffer(_instanceBuffer, 0, instanceUploadBuffer, 0, uploadBufferDesc.size);
        }
    }

    _mapObjectRenderer->ExecuteLoad();
    _complexModelRenderer->ExecuteLoad();

    // Load Water
    //_waterRenderer->LoadWater(_loadedChunks);

    return true;
}

void TerrainRenderer::LoadChunk(const ChunkToBeLoaded& chunkToBeLoaded)
{
    Terrain::Map& map = *chunkToBeLoaded.map;
    u16 chunkPosX = chunkToBeLoaded.chunkPosX;
    u16 chunkPosY = chunkToBeLoaded.chunkPosY;
    u16 chunkID = chunkToBeLoaded.chunkID;
    const Terrain::Chunk& chunk = *chunkToBeLoaded.chunk;

    StringTable& stringTable = map.stringTables[chunkID];
    entt::registry* registry = ServiceLocator::GetGameRegistry();     
    TextureSingleton& textureSingleton = registry->ctx<TextureSingleton>();

    size_t currentChunkIndex = 0;
    _loadedChunks.WriteLock(
        [&currentChunkIndex, chunkID](std::vector<u16>& loadedChunks)
        {
            currentChunkIndex = loadedChunks.size();
            loadedChunks.push_back(chunkID);
        }
    );

    // Upload cell data.
    {
        ZoneScopedN("Upload CellData");

        size_t size = sizeof(TerrainCellData) * Terrain::MAP_CELLS_PER_CHUNK;
        const u64 cellBufferOffset = (currentChunkIndex * Terrain::MAP_CELLS_PER_CHUNK) * sizeof(TerrainCellData);
        auto uploadBuffer = _renderer->CreateUploadBuffer(_cellBuffer, cellBufferOffset, size);

        u32 chunkVertexOffset = static_cast<u32>(currentChunkIndex) * Terrain::NUM_VERTICES_PER_CHUNK;
        TerrainCellData* cellDatas = static_cast<TerrainCellData*>(uploadBuffer->mappedMemory);

        // Clear memory
        memset(cellDatas, 0, size);

        // Loop over all the cells in the chunk
        for (u32 i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
        {
            ZoneScopedN("Cell");
            const Terrain::Cell& cell = chunk.cells[i];

            TerrainCellData& cellData = cellDatas[i];
            cellData.hole = cell.hole;
            cellData._padding = 1337;

            u8 layerCount = 0;
            for (auto layer : cell.layers)
            {
                if (layer.textureId == Terrain::LayerData::TextureIdInvalid)
                {
                    break;
                }

                const std::string& texturePath = textureSingleton.textureHashToPath[layer.textureId];

                Renderer::TextureDesc textureDesc;
                textureDesc.path = texturePath;

                u32 diffuseID = 0;
                {
                    ZoneScopedN("LoadTexture");
                    _renderer->LoadTextureIntoArray(textureDesc, _terrainColorTextureArray, diffuseID);
                }

                if (diffuseID > 4096)
                {
                    DebugHandler::PrintFatal("This is bad!");
                }

                cellData.diffuseIDs[layerCount++] = diffuseID;
            }
        }
    }

    u32 alphaMapStringID = chunk.alphaMapStringID;
    u32 alphaID = 0;

    if (alphaMapStringID < stringTable.GetNumStrings())
    {
        Renderer::TextureDesc chunkAlphaMapDesc;
        chunkAlphaMapDesc.path = "Data/extracted/" + stringTable.GetString(alphaMapStringID);

        {
            _renderer->LoadTextureIntoArray(chunkAlphaMapDesc, _terrainAlphaTextureArray, alphaID);
        }
    }

    // Upload chunk data.
    {
        ZoneScopedN("Upload ChunkData");

        size_t size = sizeof(TerrainChunkData);
        const u64 chunkBufferOffset = currentChunkIndex * sizeof(TerrainChunkData);
        auto uploadBuffer = _renderer->CreateUploadBuffer(_chunkBuffer, chunkBufferOffset, size);

        TerrainChunkData* chunkData = static_cast<TerrainChunkData*>(uploadBuffer->mappedMemory);
        chunkData->alphaMapID = alphaID;
    }

    // Upload height data.
    {
        ZoneScopedN("Upload HeightData");

        size_t size = sizeof(TerrainVertex) * Terrain::NUM_VERTICES_PER_CHUNK;
        const u64 chunkVertexBufferOffset = currentChunkIndex * sizeof(TerrainVertex) * Terrain::NUM_VERTICES_PER_CHUNK;
        auto uploadBuffer = _renderer->CreateUploadBuffer(_vertexBuffer, chunkVertexBufferOffset, size);

        TerrainVertex* vertexBufferMemory = reinterpret_cast<TerrainVertex*>(uploadBuffer->mappedMemory);
        for (size_t i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
        {
            size_t cellOffset = i * Terrain::MAP_CELL_TOTAL_GRID_SIZE;
            for (size_t j = 0; j < Terrain::MAP_CELL_TOTAL_GRID_SIZE; j++)
            {
                size_t offset = cellOffset + j;

                // Set height
                f32 height = chunk.cells[i].heightData[j];
                vertexBufferMemory[offset].height = height;

                u8 x = chunk.cells[i].normalData[j][0];
                u8 y = chunk.cells[i].normalData[j][1];
                u8 z = chunk.cells[i].normalData[j][2];

                // Set normal
                vertexBufferMemory[offset].normal[0] = x;
                vertexBufferMemory[offset].normal[1] = y;
                vertexBufferMemory[offset].normal[2] = z;

                // Set color
                vertexBufferMemory[offset].color[0] = chunk.cells[i].colorData[j][0];
                vertexBufferMemory[offset].color[1] = chunk.cells[i].colorData[j][1];
                vertexBufferMemory[offset].color[2] = chunk.cells[i].colorData[j][2];
            }
        }
    }

    // Calculate bounding boxes and upload height ranges
    {
        ZoneScopedN("Calculate Bounding Boxes");

        constexpr float halfWorldSize = 17066.66656f;

        vec2 chunkOrigin;
        chunkOrigin.x = halfWorldSize - (chunkPosX * Terrain::MAP_CHUNK_SIZE);
        chunkOrigin.y = halfWorldSize - (chunkPosY * Terrain::MAP_CHUNK_SIZE);

        std::vector<TerrainCellHeightRange> heightRanges;
        heightRanges.reserve(Terrain::MAP_CELLS_PER_CHUNK);

        for (u32 cellIndex = 0; cellIndex < Terrain::MAP_CELLS_PER_CHUNK; cellIndex++)
        {
            const Terrain::Cell& cell = chunk.cells[cellIndex];
            const auto minmax = std::minmax_element(cell.heightData, cell.heightData + Terrain::MAP_CELL_TOTAL_GRID_SIZE);

            const u16 cellX = cellIndex % Terrain::MAP_CELLS_PER_CHUNK_SIDE;
            const u16 cellY = cellIndex / Terrain::MAP_CELLS_PER_CHUNK_SIDE;

            vec3 min;
            vec3 max;

            // The reason for the flip in X and Y here is because in 2D X is Left and Right, Y is Forward and Backward.
            // In our 3D coordinate space X is Forward and Backwards, Y is Left and Right.

            min.x = chunkOrigin.y - (cellY * Terrain::MAP_CELL_SIZE);
            min.y = chunkOrigin.x - (cellX * Terrain::MAP_CELL_SIZE);
            min.z = *minmax.first;

            max.x = chunkOrigin.y - ((cellY + 1) * Terrain::MAP_CELL_SIZE);
            max.y = chunkOrigin.x - ((cellX + 1) * Terrain::MAP_CELL_SIZE);
            max.z = *minmax.second;

            Geometry::AABoundingBox& boundingBox = _cellBoundingBoxes.EmplaceBack();
            boundingBox.min = glm::max(min, max);
            boundingBox.max = glm::min(min, max);

            TerrainCellHeightRange heightRange;
#if USE_PACKED_HEIGHT_RANGE
            float packedHeightRange[4];
            _mm_store_ps(packedHeightRange, _mm_castsi128_ps(_mm_cvtps_ph(_mm_setr_ps(*minmax.first, *minmax.second, 0.0f, 0.0f), 0)));
            heightRange.minmax = *(u32*)packedHeightRange;
#else
            heightRange.min = *minmax.first;
            heightRange.max = *minmax.second;
#endif
            heightRanges.push_back(heightRange);
        }

        // Upload height ranges
        {
            size_t size = sizeof(TerrainCellHeightRange) * Terrain::MAP_CELLS_PER_CHUNK;
            const u64 chunkVertexBufferOffset = currentChunkIndex * sizeof(TerrainCellHeightRange) * Terrain::MAP_CELLS_PER_CHUNK;
            auto uploadBuffer = _renderer->CreateUploadBuffer(_cellHeightRangeBuffer, chunkVertexBufferOffset, size);

            memcpy(uploadBuffer->mappedMemory, heightRanges.data(), size);
        }
    }

    // TODO: Parallelize loading in the other renderers so we don't have to do this... But lets split this task into smaller chunks.
    {
        std::scoped_lock lock(_subLoadMutex);

        ZoneScopedN("Subload");
        _mapObjectRenderer->RegisterMapObjectsToBeLoaded(chunkID, chunk, stringTable);
        _complexModelRenderer->RegisterLoadFromChunk(chunkID, chunk, stringTable);
    }
}
