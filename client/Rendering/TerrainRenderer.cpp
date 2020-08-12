#include "TerrainRenderer.h"
#include "DebugRenderer.h"
#include "MapObjectRenderer.h"
#include <entt.hpp>
#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/MapSingleton.h"

#include <Renderer/Renderer.h>
#include <glm/gtc/matrix_transform.hpp>
#include <tracy/TracyVulkan.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>

#include "Camera.h"

#define USE_PACKED_HEIGHT_RANGE 1

static bool s_cullingEnabled = true;
static bool s_gpuCullingEnabled = true;
static bool s_lockCullingFrustum = false;

struct TerrainChunkData
{
    u32 alphaMapID = 0;
};

struct TerrainCellData
{
    u8 diffuseIDs[4];
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

TerrainRenderer::TerrainRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer)
    : _renderer(renderer)
    , _debugRenderer(debugRenderer)
{
    _mapObjectRenderer = new MapObjectRenderer(renderer); // Needs to be created before CreatePermanentResources
    CreatePermanentResources();

    ServiceLocator::GetInputManager()->RegisterKeybind("ToggleCulling", GLFW_KEY_F2, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        s_cullingEnabled = !s_cullingEnabled;
        return true;
    });

    ServiceLocator::GetInputManager()->RegisterKeybind("ToggleGPUCulling", GLFW_KEY_F3, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        s_gpuCullingEnabled = !s_gpuCullingEnabled;
        return true;
    });

    ServiceLocator::GetInputManager()->RegisterKeybind("ToggleLockCullingFrustum", GLFW_KEY_F5, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        s_lockCullingFrustum = !s_lockCullingFrustum;
        return true;
    });
}

TerrainRenderer::~TerrainRenderer()
{
    delete _mapObjectRenderer;
}

void TerrainRenderer::Update(f32 deltaTime, const Camera& camera)
{
    //for (const BoundingBox& boundingBox : _cellBoundingBoxes)
    //{
    //    _debugRenderer->DrawAABB3D(boundingBox.min, boundingBox.max, 0xff00ff00);
    //}

    if (s_cullingEnabled && !s_gpuCullingEnabled)
    {
        CPUCulling(camera);
    }

    // Subrenderers
    _mapObjectRenderer->Update(deltaTime);
}

__forceinline bool IsInsideFrustum(const vec4* planes, const BoundingBox& boundingBox)
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

void TerrainRenderer::CPUCulling(const Camera& camera)
{
    ZoneScoped;

    static vec4 frustumPlanes[6];
    static mat4x4 lockedViewProjectionMatrix;

    if (!s_lockCullingFrustum)
    {
        memcpy(frustumPlanes, camera.GetFrustumPlanes(), sizeof(frustumPlanes));
        lockedViewProjectionMatrix = camera.GetViewProjectionMatrix();
    }

    _culledInstances.clear();
    _culledInstances.reserve(_loadedChunks.size() * Terrain::MAP_CELLS_PER_CHUNK);

    const size_t chunkCount = _loadedChunks.size();
    size_t boundingBoxIndex = 0;
    for (size_t i = 0; i < chunkCount; ++i)
    {
        for (u16 cellId = 0; cellId < Terrain::MAP_CELLS_PER_CHUNK; ++cellId)
        {
            const BoundingBox& boundingBox = _cellBoundingBoxes[boundingBoxIndex++];
            if (IsInsideFrustum(frustumPlanes, boundingBox))
            {
                const u16 chunkId = _loadedChunks[i];
                _culledInstances.push_back((chunkId << 16) | cellId);
            }
        }
    }

    _debugRenderer->DrawFrustum(lockedViewProjectionMatrix, 0xff0000ff);
}

void TerrainRenderer::AddTerrainDepthPrepass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
    return;

    // Terrain Depth Prepass
    {
        struct TerrainDepthPrepassData
        {
            Renderer::RenderPassMutableResource mainDepth;
        };
        renderGraph->AddPass<TerrainDepthPrepassData>("TerrainDepth",
            [=](TerrainDepthPrepassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
            
            return true;// Return true from setup to enable this pass, return false to disable it
        },
            [=](TerrainDepthPrepassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
        {
            GPU_SCOPED_PROFILER_ZONE(commandList, TerrainDepth);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            resources.InitializePipelineDesc(pipelineDesc);
            
            // Shader
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/terrain.vs.hlsl.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            // Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
            pipelineDesc.states.inputLayouts[0].enabled = true;
            pipelineDesc.states.inputLayouts[0].SetName("INSTANCEID");
            pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::INPUT_FORMAT_R32_UINT;
            pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_INSTANCE;

            // Depth state
            pipelineDesc.states.depthStencilState.depthEnable = true;
            pipelineDesc.states.depthStencilState.depthWriteEnable = true;
            pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_LESS;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;
            pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

            pipelineDesc.depthStencil = data.mainDepth;

            // Set pipeline
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            // Set instance buffer
            commandList.SetBuffer(0, _instanceBuffer);

            // Set index buffer
            commandList.SetIndexBuffer(_cellIndexBuffer, Renderer::IndexFormat::UInt16);

            // Bind viewbuffer
            _passDescriptorSet.Bind("ViewData"_h, viewConstantBuffer->GetBuffer(frameIndex));
            _passDescriptorSet.Bind("_vertexHeights"_h, _vertexBuffer);
            _passDescriptorSet.Bind("_cellData"_h, _cellBuffer);
            _passDescriptorSet.Bind("_cellDataVS"_h, _cellBuffer);
            _passDescriptorSet.Bind("_chunkData"_h, _chunkBuffer);

            // Bind descriptorset
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);
            commandList.DrawIndexed(Terrain::NUM_INDICES_PER_CELL, Terrain::MAP_CELLS_PER_CHUNK * (u32)_loadedChunks.size(), 0, 0, 0);
            //commandList.DrawIndexedIndirect(_argumentBuffer, 0, 1 );

            commandList.EndPipeline(pipeline);
        });
    }

    // Subrenderers
    _mapObjectRenderer->AddMapObjectDepthPrepass(renderGraph, viewConstantBuffer, depthTarget, frameIndex);
}

void TerrainRenderer::AddTerrainPass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex, u8 debugMode, const Camera& camera)
{
    // Terrain Pass
    {
        struct TerrainPassData
        {
            Renderer::RenderPassMutableResource mainColor;
            Renderer::RenderPassMutableResource mainDepth;
        };

        renderGraph->AddPass<TerrainPassData>("Terrain Pass",
            [=](TerrainPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
            data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
            [=](TerrainPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
        {
            GPU_SCOPED_PROFILER_ZONE(commandList, TerrainPass);

            // Upload culled instances
            if (s_cullingEnabled && !s_gpuCullingEnabled && !_culledInstances.empty())
            {
                Renderer::BufferDesc uploadBufferDesc;
                uploadBufferDesc.name = "TerrainInstanceUploadBuffer";
                uploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
                uploadBufferDesc.size = sizeof(u32) * _culledInstances.size();
                uploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;

                Renderer::BufferID instanceUploadBuffer = _renderer->CreateBuffer(uploadBufferDesc);
                _renderer->QueueDestroyBuffer(instanceUploadBuffer);
                
                void* instanceBufferMemory = _renderer->MapBuffer(instanceUploadBuffer);
                memcpy(instanceBufferMemory, _culledInstances.data(), uploadBufferDesc.size);
                _renderer->UnmapBuffer(instanceUploadBuffer);
                commandList.CopyBuffer(_culledInstanceBuffer, 0, instanceUploadBuffer, 0, uploadBufferDesc.size);

                commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToIndirectArguments, _culledInstanceBuffer);
            }

            // Cull instances on GPU
            if (s_cullingEnabled && s_gpuCullingEnabled)
            {
                Renderer::ComputePipelineDesc pipelineDesc;
                resources.InitializePipelineDesc(pipelineDesc);

                Renderer::ComputeShaderDesc shaderDesc;
                shaderDesc.path = "Data/shaders/terrainCulling.cs.hlsl.spv";
                pipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);

                Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(pipelineDesc);
                commandList.BindPipeline(pipeline);

                if (!s_lockCullingFrustum)
                {
                    memcpy(_cullingConstantBuffer->resource.frustumPlanes, camera.GetFrustumPlanes(), sizeof(vec4[6]));
                    _cullingConstantBuffer->Apply(frameIndex);
                }

                _cullingPassDescriptorSet.Bind("_instances", _instanceBuffer);
                _cullingPassDescriptorSet.Bind("_heightRanges", _cellHeightRangeBuffer);
                _cullingPassDescriptorSet.Bind("_culledInstances", _culledInstanceBuffer);
                _cullingPassDescriptorSet.Bind("_argumentBuffer", _argumentBuffer);
                _cullingPassDescriptorSet.Bind("_constants", _cullingConstantBuffer->GetBuffer(frameIndex));

                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_cullingPassDescriptorSet, frameIndex);

                const u32 cellCount = (u32)_loadedChunks.size() * Terrain::MAP_CELLS_PER_CHUNK;
                commandList.Dispatch((cellCount + 31) / 32, 1, 1);

                commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _culledInstanceBuffer);
                commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _argumentBuffer);
            }

            // Upload culled instances
            if (s_cullingEnabled && !s_gpuCullingEnabled && !_culledInstances.empty())
            {
                Renderer::BufferDesc uploadBufferDesc;
                uploadBufferDesc.name = "TerrainInstanceUploadBuffer";
                uploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
                uploadBufferDesc.size = sizeof(u32) * _culledInstances.size();
                uploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;

                Renderer::BufferID instanceUploadBuffer = _renderer->CreateBuffer(uploadBufferDesc);
                _renderer->QueueDestroyBuffer(instanceUploadBuffer);
                
                void* instanceBufferMemory = _renderer->MapBuffer(instanceUploadBuffer);
                memcpy(instanceBufferMemory, _culledInstances.data(), uploadBufferDesc.size);
                _renderer->UnmapBuffer(instanceUploadBuffer);
                commandList.CopyBuffer(_culledInstanceBuffer, 0, instanceUploadBuffer, 0, uploadBufferDesc.size);

                commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToIndirectArguments, _culledInstanceBuffer);
            }

            // Cull instances on GPU
            if (s_cullingEnabled && s_gpuCullingEnabled)
            {
                Renderer::ComputePipelineDesc pipelineDesc;
                resources.InitializePipelineDesc(pipelineDesc);

                Renderer::ComputeShaderDesc shaderDesc;
                shaderDesc.path = "Data/shaders/terrainCulling.cs.hlsl.spv";
                pipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);

                Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(pipelineDesc);
                commandList.BindPipeline(pipeline);

                if (!s_lockCullingFrustum)
                {
                    memcpy(_cullingConstantBuffer->resource.frustumPlanes, camera.GetFrustumPlanes(), sizeof(vec4[6]));
                    _cullingConstantBuffer->Apply(frameIndex);
                }

                _cullingPassDescriptorSet.Bind("_instances", _instanceBuffer);
                _cullingPassDescriptorSet.Bind("_heightRanges", _cellHeightRangeBuffer);
                _cullingPassDescriptorSet.Bind("_culledInstances", _culledInstanceBuffer);
                _cullingPassDescriptorSet.Bind("_argumentBuffer", _argumentBuffer);
                _cullingPassDescriptorSet.Bind("_constants", _cullingConstantBuffer->GetBuffer(frameIndex));

                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_cullingPassDescriptorSet, frameIndex);

                const u32 cellCount = (u32)_loadedChunks.size() * Terrain::MAP_CELLS_PER_CHUNK;
                commandList.Dispatch((cellCount + 31) / 32, 1, 1);

                commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _culledInstanceBuffer);
                commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _argumentBuffer);
            }

            Renderer::GraphicsPipelineDesc pipelineDesc;
            resources.InitializePipelineDesc(pipelineDesc);

            // Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/terrain.vs.hlsl.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = (debugMode == 0) ? "Data/shaders/terrain.ps.hlsl.spv" : "Data/shaders/terrainDebug.ps.hlsl.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            // Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
            pipelineDesc.states.inputLayouts[0].enabled = true;
            pipelineDesc.states.inputLayouts[0].SetName("INSTANCEID");
            pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::INPUT_FORMAT_R32_UINT;
            pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_INSTANCE;

            // Depth state
            pipelineDesc.states.depthStencilState.depthEnable = true;
            pipelineDesc.states.depthStencilState.depthWriteEnable = true;
            pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_LESS;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;
            pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

            // Render targets
            pipelineDesc.renderTargets[0] = data.mainColor;

            pipelineDesc.depthStencil = data.mainDepth;

            // Set pipeline
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            // Set instance buffer
            const Renderer::BufferID instanceBuffer = s_cullingEnabled ? _culledInstanceBuffer : _instanceBuffer;
            commandList.SetBuffer(0, instanceBuffer);

            // Set index buffer
            commandList.SetIndexBuffer(_cellIndexBuffer, Renderer::IndexFormat::UInt16);

            // Bind viewbuffer
            _passDescriptorSet.Bind("ViewData"_h, viewConstantBuffer->GetBuffer(frameIndex));
            _passDescriptorSet.Bind("_vertexHeights"_h, _vertexBuffer);
            _passDescriptorSet.Bind("_cellData"_h, _cellBuffer);
            _passDescriptorSet.Bind("_cellDataVS"_h, _cellBuffer);
            _passDescriptorSet.Bind("_chunkData"_h, _chunkBuffer);

            // Bind descriptorset
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);
            if (s_cullingEnabled)
            {
                if (s_gpuCullingEnabled)
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
                const u32 cellCount = Terrain::MAP_CELLS_PER_CHUNK * (u32)_loadedChunks.size();
                TracyPlot("Cell Instance Count", (i64)cellCount);
                commandList.DrawIndexed(Terrain::NUM_INDICES_PER_CELL, cellCount, 0, 0, 0);
            }

            commandList.EndPipeline(pipeline);
        });
    }

    // Subrenderers
    _mapObjectRenderer->AddMapObjectPass(renderGraph, viewConstantBuffer, renderTarget, depthTarget, frameIndex);
}

void TerrainRenderer::CreatePermanentResources()
{
    entt::registry* registry = ServiceLocator::GetGameRegistry();
    MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

    // Create texture array
    Renderer::TextureArrayDesc textureColorArrayDesc;
    textureColorArrayDesc.size = 4096;

    _terrainColorTextureArray = _renderer->CreateTextureArray(textureColorArrayDesc);

    Renderer::TextureArrayDesc textureAlphaArrayDesc;
    textureAlphaArrayDesc.size = Terrain::MAP_CHUNKS_PER_MAP_SIDE * Terrain::MAP_CHUNKS_PER_MAP_SIDE;

    _terrainAlphaTextureArray = _renderer->CreateTextureArray(textureAlphaArrayDesc);

    // Create and load a 1x1 pixel RGBA8 unorm texture with zero'ed data so we can use textureArray[0] as "invalid" textures, sampling it will return 0.0f on all channels
    Renderer::DataTextureDesc zeroColorTexture;
    zeroColorTexture.debugName = "TerrainZeroColor";
    zeroColorTexture.layers = 1;
    zeroColorTexture.width = 1;
    zeroColorTexture.height = 1;
    zeroColorTexture.format = Renderer::IMAGE_FORMAT_R8G8B8A8_UNORM;
    zeroColorTexture.data = new u8[4]{ 0, 0, 0, 0 };

    u32 index;
    _renderer->CreateDataTextureIntoArray(zeroColorTexture, _terrainColorTextureArray, index);

    // Samplers
    Renderer::SamplerDesc alphaSamplerDesc;
    alphaSamplerDesc.enabled = true;
    alphaSamplerDesc.filter = Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_LINEAR;
    alphaSamplerDesc.addressU = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    alphaSamplerDesc.addressV = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    alphaSamplerDesc.addressW = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    alphaSamplerDesc.shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_PIXEL;

    _alphaSampler = _renderer->CreateSampler(alphaSamplerDesc);

    Renderer::SamplerDesc colorSamplerDesc;
    colorSamplerDesc.enabled = true;
    colorSamplerDesc.filter = Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_LINEAR;
    colorSamplerDesc.addressU = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    colorSamplerDesc.addressV = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    colorSamplerDesc.addressW = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    colorSamplerDesc.shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_PIXEL;

    _colorSampler = _renderer->CreateSampler(colorSamplerDesc);

    // Descriptor sets
    _passDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _passDescriptorSet.Bind("_alphaSampler"_h, _alphaSampler);
    _passDescriptorSet.Bind("_colorSampler"_h, _colorSampler);
    _passDescriptorSet.Bind("_terrainColorTextures"_h, _terrainColorTextureArray);
    _passDescriptorSet.Bind("_terrainAlphaTextures"_h, _terrainAlphaTextureArray);

    _drawDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());

    // Culling constant buffer
    _cullingConstantBuffer = new Renderer::Buffer<CullingConstants>(_renderer, "CullingConstantBuffer", Renderer::BUFFER_USAGE_UNIFORM_BUFFER, Renderer::BufferCPUAccess::WriteOnly);

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainCellIndexBuffer";
        desc.size = Terrain::NUM_INDICES_PER_CELL * sizeof(u16);
        desc.usage = Renderer::BUFFER_USAGE_INDEX_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _cellIndexBuffer = _renderer->CreateBuffer(desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "CulledTerrainInstanceBuffer";
        desc.size = sizeof(u32) * Terrain::MAP_CELLS_PER_CHUNK * (Terrain::MAP_CHUNKS_PER_MAP_SIDE * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_VERTEX_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _instanceBuffer = _renderer->CreateBuffer(desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainInstanceBuffer";
        desc.size = sizeof(u32) * Terrain::MAP_CELLS_PER_CHUNK * (Terrain::MAP_CHUNKS_PER_MAP_SIDE * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_VERTEX_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _culledInstanceBuffer = _renderer->CreateBuffer(desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainArgumentBuffer";
        desc.size = sizeof(VkDrawIndexedIndirectCommand);
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER;
        _argumentBuffer = _renderer->CreateBuffer(desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainChunkBuffer";
        desc.size = sizeof(TerrainChunkData) * (Terrain::MAP_CHUNKS_PER_MAP_SIDE * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _chunkBuffer = _renderer->CreateBuffer(desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainCellBuffer";
        desc.size = sizeof(TerrainCellData) * Terrain::MAP_CELLS_PER_CHUNK * (Terrain::MAP_CHUNKS_PER_MAP_SIDE * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _cellBuffer = _renderer->CreateBuffer(desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainVertexBuffer";
        desc.size = sizeof(f32) * Terrain::NUM_VERTICES_PER_CHUNK * (Terrain::MAP_CHUNKS_PER_MAP_SIDE * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _vertexBuffer = _renderer->CreateBuffer(desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "CellHeightRangeBuffer";
        desc.size = sizeof(TerrainCellHeightRange) * Terrain::MAP_CELLS_PER_CHUNK * (Terrain::MAP_CHUNKS_PER_MAP_SIDE * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _cellHeightRangeBuffer = _renderer->CreateBuffer(desc);
    }

    // Upload cell index buffer
    {
        Renderer::BufferDesc indexUploadBufferDesc;
        indexUploadBufferDesc.name = "TerrainCellIndexUploadBuffer";
        indexUploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
        indexUploadBufferDesc.size = sizeof(u16) * Terrain::NUM_INDICES_PER_CELL;
        indexUploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;

        Renderer::BufferID indexUploadBuffer = _renderer->CreateBuffer(indexUploadBufferDesc);
        _renderer->QueueDestroyBuffer(indexUploadBuffer);

        u16* indices = static_cast<u16*>(_renderer->MapBuffer(indexUploadBuffer));

        // Fill index buffer
        size_t indexIndex = 0;
        for (u16 row = 0; row < Terrain::CELL_INNER_GRID_SIDE; row++)
        {
            for (u16 col = 0; col < Terrain::CELL_INNER_GRID_SIDE; col++)
            {
                const u16 baseVertex = (row * Terrain::CELL_TOTAL_GRID_SIDE + col);

                //1     2
                //   0
                //3     4

                const u16 topLeftVertex = baseVertex;
                const u16 topRightVertex = baseVertex + 1;
                const u16 bottomLeftVertex = baseVertex + Terrain::CELL_TOTAL_GRID_SIDE;
                const u16 bottomRightVertex = baseVertex + Terrain::CELL_TOTAL_GRID_SIDE + 1;
                const u16 centerVertex = baseVertex + Terrain::CELL_OUTER_GRID_SIDE;

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

    // Load map
    Terrain::Map& map = mapSingleton.maps[0];
    LoadChunksAround(map, ivec2(32, 32), 32); // Goldshire
    //LoadChunksAround(map, ivec2(40, 32), 8); // Razor Hill
    //LoadChunksAround(map, ivec2(22, 25), 8); // Borean Tundra

    //LoadChunksAround(map, ivec2(0, 0), 8); // Goldshire

    // Upload instance data
    {
        const size_t cellCount = Terrain::MAP_CELLS_PER_CHUNK * _loadedChunks.size();

        Renderer::BufferDesc uploadBufferDesc;
        uploadBufferDesc.name = "TerrainInstanceUploadBuffer";
        uploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
        uploadBufferDesc.size = sizeof(u32) * cellCount;
        uploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;

        Renderer::BufferID instanceUploadBuffer = _renderer->CreateBuffer(uploadBufferDesc);
        _renderer->QueueDestroyBuffer(instanceUploadBuffer);

        void* instanceBufferMemory = _renderer->MapBuffer(instanceUploadBuffer);
        u32* instanceData = static_cast<u32*>(instanceBufferMemory);
        u32 instanceDataIndex = 0;

        for (const u16 chunkID : _loadedChunks)
        {
            for (u32 cellID = 0; cellID < Terrain::MAP_CELLS_PER_CHUNK; ++cellID)
            {
                instanceData[instanceDataIndex++] = (chunkID << 16) | (cellID & 0xffff);
            }
        }
        assert(instanceDataIndex == cellCount);
        _renderer->UnmapBuffer(instanceUploadBuffer);
        _renderer->CopyBuffer(_instanceBuffer, 0, instanceUploadBuffer, 0, uploadBufferDesc.size);
    }
}

static inline u16 transpose4x4BitMatrix(u16 in)
{
    u16 out = 0;
    for (u16 i = 0; i < 16; ++i)
    {
        const u16 set = (in >> i) & 1;
        const u16 outIndex = ((i * 4) + (i / 4)) % 16;
        out |= (set << outIndex);
    }
    return out;
}

void TerrainRenderer::LoadChunk(Terrain::Map& map, u16 chunkPosX, u16 chunkPosY)
{
    u16 chunkId;
    map.GetChunkIdFromChunkPosition(chunkPosX, chunkPosY, chunkId);

    const auto chunkIt = map.chunks.find(chunkId);
    if (chunkIt == map.chunks.cend())
    {
        return;
    }

    const Terrain::Chunk& chunk = chunkIt->second;
    StringTable& stringTable = map.stringTables[chunkId];

    // Upload cell data.
    {
        Renderer::BufferDesc cellDataUploadBufferDesc;
        cellDataUploadBufferDesc.name = "TerrainCellUploadBuffer";
        cellDataUploadBufferDesc.size = sizeof(TerrainCellData) * Terrain::MAP_CELLS_PER_CHUNK;
        cellDataUploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;
        cellDataUploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID cellUploadBuffer = _renderer->CreateBuffer(cellDataUploadBufferDesc);
        _renderer->QueueDestroyBuffer(cellUploadBuffer);

        TerrainCellData* cellDatas = static_cast<TerrainCellData*>(_renderer->MapBuffer(cellUploadBuffer));

        // Loop over all the cells in the chunk
        for (u32 i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
        {
            const Terrain::Cell& cell = chunk.cells[i];

            TerrainCellData& cellData = cellDatas[i];
            //cellData.hole = transpose4x4BitMatrix(cell.hole);
            cellData.hole = cell.hole;
            cellData._padding = 1337;

            u8 layerCount = 0;
            for (auto layer : cell.layers)
            {
                if (layer.textureId == Terrain::LayerData::TextureIdInvalid)
                    break;

                const std::string& texturePath = stringTable.GetString(layer.textureId);
                
                Renderer::TextureDesc textureDesc;
                textureDesc.path = "Data/extracted/Textures/" + texturePath;

                u32 diffuseID = 0;
                _renderer->LoadTextureIntoArray(textureDesc, _terrainColorTextureArray, diffuseID);
                assert(diffuseID < 256);

                cellData.diffuseIDs[layerCount++] = diffuseID;
            }
        }

        _renderer->UnmapBuffer(cellUploadBuffer);
        const u64 cellBufferOffset = (static_cast<u64>(chunkId) * Terrain::MAP_CELLS_PER_CHUNK) * sizeof(TerrainCellData);
        _renderer->CopyBuffer(_cellBuffer, cellBufferOffset, cellUploadBuffer, 0, cellDataUploadBufferDesc.size);
    }

    u32 alphaMapStringID = chunk.alphaMapStringID;
    u32 alphaID = 0;

    if (alphaMapStringID < stringTable.GetNumStrings())
    {
        Renderer::TextureDesc chunkAlphaMapDesc;
        chunkAlphaMapDesc.path = stringTable.GetString(alphaMapStringID);

        _renderer->LoadTextureIntoArray(chunkAlphaMapDesc, _terrainAlphaTextureArray, alphaID);
    }
    
    // Upload chunk data.
    {
        Renderer::BufferDesc chunkDataUploadBufferDesc;
        chunkDataUploadBufferDesc.name = "TerrainChunkUploadBuffer";
        chunkDataUploadBufferDesc.size = sizeof(TerrainChunkData);
        chunkDataUploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;
        chunkDataUploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID chunkUploadBuffer = _renderer->CreateBuffer(chunkDataUploadBufferDesc);
        _renderer->QueueDestroyBuffer(chunkUploadBuffer);

        TerrainChunkData* chunkData = static_cast<TerrainChunkData*>(_renderer->MapBuffer(chunkUploadBuffer));
        chunkData->alphaMapID = alphaID;

        _renderer->UnmapBuffer(chunkUploadBuffer);
        const u64 chunkBufferOffset = static_cast<u64>(chunkId) * sizeof(TerrainChunkData);
        _renderer->CopyBuffer(_chunkBuffer, chunkBufferOffset, chunkUploadBuffer, 0, chunkDataUploadBufferDesc.size);
    }

    // Upload height data.
    {
        Renderer::BufferDesc vertexUploadBufferDesc;
        vertexUploadBufferDesc.name = "TerrainVertexUploadBuffer";
        vertexUploadBufferDesc.size = sizeof(f32) * Terrain::NUM_VERTICES_PER_CHUNK;
        vertexUploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;
        vertexUploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID vertexUploadBuffer = _renderer->CreateBuffer(vertexUploadBufferDesc);
        _renderer->QueueDestroyBuffer(vertexUploadBuffer);

        void* vertexBufferMemory = _renderer->MapBuffer(vertexUploadBuffer);
        for (size_t i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; ++i)
        {
            void* dstVertices = static_cast<u8*>(vertexBufferMemory) + (i * Terrain::CELL_TOTAL_GRID_SIZE * sizeof(f32));
            const void* srcVertices = chunk.cells[i].heightData;
            memcpy(dstVertices, srcVertices, Terrain::CELL_TOTAL_GRID_SIZE * sizeof(f32));
        }

        _renderer->UnmapBuffer(vertexUploadBuffer);
        const u64 chunkVertexBufferOffset = chunkId * sizeof(f32) * Terrain::NUM_VERTICES_PER_CHUNK;
        _renderer->CopyBuffer(_vertexBuffer, chunkVertexBufferOffset, vertexUploadBuffer, 0, vertexUploadBufferDesc.size);
    }

    // Calculate bounding boxes and upload height ranges
    {
        constexpr float halfWorldSize = 17066.66656f;

        vec2 chunkOrigin;
        chunkOrigin.x = -((chunkPosY)*Terrain::MAP_CHUNK_SIZE - halfWorldSize);
        chunkOrigin.y = ((Terrain::MAP_CHUNKS_PER_MAP_SIDE - chunkPosX) * Terrain::MAP_CHUNK_SIZE - halfWorldSize);

        std::vector<TerrainCellHeightRange> heightRanges;
        heightRanges.reserve(Terrain::MAP_CELLS_PER_CHUNK);

        for (u32 cellIndex = 0; cellIndex < Terrain::MAP_CELLS_PER_CHUNK; cellIndex++)
        {
            const Terrain::Cell& cell = chunk.cells[cellIndex];
            const auto minmax = std::minmax_element(cell.heightData, cell.heightData + Terrain::CELL_TOTAL_GRID_SIZE);

            const u16 cellX = cellIndex % Terrain::MAP_CELLS_PER_CHUNK_SIDE;
            const u16 cellY = cellIndex / Terrain::MAP_CELLS_PER_CHUNK_SIDE;

            vec3 min;
            vec3 max;

            min.x = chunkOrigin.x - (cellY * Terrain::CELL_SIZE);
            min.y = *minmax.first;
            min.z = chunkOrigin.y - (cellX * Terrain::CELL_SIZE);

            max.x = chunkOrigin.x - ((cellY + 1) * Terrain::CELL_SIZE);
            max.y = *minmax.second;
            max.z = chunkOrigin.y - ((cellX + 1) * Terrain::CELL_SIZE);

            BoundingBox boundingBox;
            boundingBox.min = glm::max(min, max);
            boundingBox.max = glm::min(min, max);
            _cellBoundingBoxes.push_back(boundingBox);

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
            Renderer::BufferDesc heightRangeUploadBufferDesc;
            heightRangeUploadBufferDesc.name = "HeightRangeUploadBuffer";
            heightRangeUploadBufferDesc.size = sizeof(TerrainCellHeightRange) * Terrain::MAP_CELLS_PER_CHUNK;
            heightRangeUploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;
            heightRangeUploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

            Renderer::BufferID heightRangeUploadBuffer = _renderer->CreateBuffer(heightRangeUploadBufferDesc);
            _renderer->QueueDestroyBuffer(heightRangeUploadBuffer);

            void* heightRangeBufferMemory = _renderer->MapBuffer(heightRangeUploadBuffer);
            memcpy(heightRangeBufferMemory, heightRanges.data(), heightRangeUploadBufferDesc.size);
            _renderer->UnmapBuffer(heightRangeUploadBuffer);

            const u64 chunkInstanceIndex = _loadedChunks.size();
            const u64 chunkVertexBufferOffset = chunkInstanceIndex * sizeof(TerrainCellHeightRange) * Terrain::MAP_CELLS_PER_CHUNK;
            _renderer->CopyBuffer(_cellHeightRangeBuffer, chunkVertexBufferOffset, heightRangeUploadBuffer, 0, heightRangeUploadBufferDesc.size);
        }
    }

    _loadedChunks.push_back(chunkId);
}

void TerrainRenderer::LoadChunksAround(Terrain::Map& map, ivec2 middleChunk, u16 drawDistance)
{
    // Middle position has to be within map grid
    assert(middleChunk.x >= 0);
    assert(middleChunk.y >= 0);
    assert(middleChunk.x < 64);
    assert(middleChunk.y < 64);

    assert(drawDistance > 0);
    assert(drawDistance <= 64);

    u16 radius = drawDistance-1;

    ivec2 startPos = ivec2(middleChunk.x - radius, middleChunk.y - radius);
    startPos = glm::max(startPos, ivec2(0, 0));

    ivec2 endPos = ivec2(middleChunk.x + radius, middleChunk.y + radius);
    endPos = glm::min(endPos, ivec2(63, 63));

    for(i32 y = startPos.y; y < endPos.y; y++)
    {
        for (i32 x = startPos.x; x < endPos.x; x++)
        {
            LoadChunk(map, x, y);
        }
    }
}
