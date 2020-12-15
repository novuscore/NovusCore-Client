#include "WaterRenderer.h"
#include "DebugRenderer.h"
#include "../Utils/ServiceLocator.h"

#include <filesystem>
#include <GLFW/glfw3.h>

#include <InputManager.h>
#include <Renderer/Renderer.h>
#include <Utils/FileReader.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "../ECS/Components/Singletons/MapSingleton.h"

#include <tracy/TracyVulkan.hpp>

namespace fs = std::filesystem;

WaterRenderer::WaterRenderer(Renderer::Renderer* renderer)
    : _renderer(renderer)
{
    CreatePermanentResources();
}

WaterRenderer::~WaterRenderer()
{

}

void WaterRenderer::Update(f32 deltaTime)
{
    static f32 updateTimer = 0;

    if (updateTimer >= 1 / 30.f)
    {
        if (++_textureIndex == 30)
        {
            _textureIndex = 0;
        }

        updateTimer -= 1 / 30.f;
    }
    else
        updateTimer += deltaTime;
}

void WaterRenderer::LoadWater(const std::vector<u16>& chunkIDs)
{
    RegisterChunksToBeLoaded(chunkIDs);
    ExecuteLoad();
}

void WaterRenderer::Clear()
{
    if (_loadedWater.size() > 0)
    {
        for (LoadedWater& loadedWater : _loadedWater)
        {
            _renderer->QueueDestroyBuffer(loadedWater.instanceBuffer);
        }
    }

    _loadedWater.clear();

    //_renderer->UnloadTexturesInArray(_waterTextures, 0);
}

void WaterRenderer::AddWaterPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
    struct WaterPassData
    {
        Renderer::RenderPassMutableResource mainColor;
        Renderer::RenderPassMutableResource mainDepth;
    };

    const auto setup = [=](WaterPassData& data, Renderer::RenderGraphBuilder& builder)
    {
        data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
        data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);

        return true; // Return true from setup to enable this pass, return false to disable it
    };

    const auto execute = [=](WaterPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList)
    {
        GPU_SCOPED_PROFILER_ZONE(commandList, WaterPass);

        Renderer::GraphicsPipelineDesc pipelineDesc;
        resources.InitializePipelineDesc(pipelineDesc);

        // Shaders
        Renderer::VertexShaderDesc vertexShaderDesc;
        vertexShaderDesc.path = "Data/shaders/water.vs.hlsl.spv";
        pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

        Renderer::PixelShaderDesc pixelShaderDesc;
        pixelShaderDesc.path = "Data/shaders/water.ps.hlsl.spv";
        pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

        // Depth state
        pipelineDesc.states.depthStencilState.depthEnable = true;
        pipelineDesc.states.depthStencilState.depthWriteEnable = true;
        pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_GREATER;

        // Rasterizer state
        pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_NONE; //Renderer::CullMode::CULL_MODE_BACK;
        pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

        // Blending state
        pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
        pipelineDesc.states.blendState.renderTargets[0].blendOp = Renderer::BlendOp::BLEND_OP_ADD;
        pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::BLEND_MODE_SRC_ALPHA;
        pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::BLEND_MODE_INV_SRC_ALPHA;

        // Render targets
        pipelineDesc.renderTargets[0] = data.mainColor;
        pipelineDesc.depthStencil = data.mainDepth;

        // Set pipeline
        Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
        commandList.BeginPipeline(pipeline);

        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

        for (LoadedWater& loadedWater : _loadedWater)
        {
            commandList.PushMarker("Water", Color::White);

            _passDescriptorSet.Bind("_instanceData", loadedWater.instanceBuffer);
            _passDescriptorSet.Bind("_vertexPositions", _vertexBuffer);
            _passDescriptorSet.Bind("_textures", _waterTextures);

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            {
                commandList.PushConstant(&_textureIndex, 0, sizeof(u32));

                commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);
                commandList.DrawIndexed(6, loadedWater.numInstances, 0, 0, 0);
            }

            commandList.PopMarker();
        }

        commandList.EndPipeline(pipeline);
    };

    renderGraph->AddPass<WaterPassData>("Water Pass", setup, execute);
}

void WaterRenderer::CreatePermanentResources()
{
    Renderer::TextureArrayDesc textureArrayDesc;
    textureArrayDesc.size = 128;

    _waterTextures = _renderer->CreateTextureArray(textureArrayDesc);

    Renderer::SamplerDesc samplerDesc;
    samplerDesc.enabled = true;
    samplerDesc.filter = Renderer::SamplerFilter::SAMPLER_FILTER_ANISOTROPIC;//Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.addressU = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressV = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressW = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_PIXEL;
    samplerDesc.maxAnisotropy = 8;

    _sampler = _renderer->CreateSampler(samplerDesc);
    _passDescriptorSet.Bind("_sampler", _sampler);

    // For now we ignore water vertex data and render it all as planes
    std::vector<vec2> vertices;
    std::vector<u16> indices;

    // Build Plane
    vertices.push_back(vec2(0, 0));
    vertices.push_back(vec2(0, -4.1666f));
    vertices.push_back(vec2(-4.1666f, 0));
    vertices.push_back(vec2(-4.1666f, -4.1666f));

    // Build Triangles for Plane
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);

    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(3);

    // -- Create Vertex Buffer --
    {
        const size_t bufferSize = 4 * sizeof(vec2);

        Renderer::BufferDesc desc;
        desc.name = "VertexBuffer";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        _vertexBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "VertexBufferStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer

        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, vertices.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_vertexBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create Index Buffer --
    {
        const size_t bufferSize = 6 * sizeof(u16);

        Renderer::BufferDesc desc;
        desc.name = "IndexBuffer";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_INDEX_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        _indexBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "IndexBufferStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer

        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, indices.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_indexBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    std::string oceanTextureFolder = "Data/extracted/Textures/xtextures/ocean/";
   
    for (u32 i = 1; i <= 30; i++)
    {
        Renderer::TextureDesc textureDesc;
        textureDesc.path = oceanTextureFolder + "ocean_h." + std::to_string(i) + ".dds";

        u32 textureId = 0;
        _renderer->LoadTextureIntoArray(textureDesc, _waterTextures, textureId);
    }
}

bool WaterRenderer::RegisterChunksToBeLoaded(const std::vector<u16>& chunkIDs)
{
    NC_LOG_MESSAGE("Loading Water");

    entt::registry* registry = ServiceLocator::GetGameRegistry();
    MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

    _loadedWater.clear();
    LoadedWater& loadedWater = _loadedWater.emplace_back();
    loadedWater.debugName = "Water";

    for (const u16& chunkID : chunkIDs)
    {
        Terrain::Chunk& chunk = mapSingleton.currentMap.chunks[chunkID];

        u16 chunkPosX = 0;
        u16 chunkPosY = 0;
        mapSingleton.currentMap.GetChunkPositionFromChunkId(chunkID, chunkPosX, chunkPosY);

        vec2 chunkWorldPos = vec2(chunkPosX, chunkPosY) * Terrain::MAP_CHUNK_SIZE;

        if (chunk.liquidHeaders.size() == 0)
            continue;

        u32 liquidInfoOffset = 0;
        u32 liquidDataOffset = 0;

        for (u32 i = 0; i < chunk.liquidHeaders.size(); i++)
        {
            Terrain::CellLiquidHeader& liquidHeader = chunk.liquidHeaders[i];

            bool hasAttributes = liquidHeader.packedData >> 7;
            u8 numInstances = liquidHeader.packedData & 0x7F;

            if (numInstances == 0)
                continue;

            u16 cellPosX = i % Terrain::MAP_CELLS_PER_CHUNK_SIDE;
            u16 cellPosY = i / Terrain::MAP_CELLS_PER_CHUNK_SIDE;
            vec2 cellWorldPos = vec2(cellPosX, cellPosY) * Terrain::MAP_CELL_SIZE;

            if (i == 230)
            {
                volatile int test = 5;
            }

            for (u32 j = 0; j < numInstances; j++)
            {
                Terrain::CellLiquidInstance& liquidInstance = chunk.liquidInstances[liquidInfoOffset + j];

                // Packed Format
                // Bit 1-6 (liquidVertexFormat)
                // Bit 7 (hasBitMaskForPatches)
                // Bit 8 (hasVertexData)

                bool hasVertexData = liquidInstance.packedData >> 7;
                bool hasBitMaskForPatches = (liquidInstance.packedData >> 6) & 0x1;
                u8 liquidVertexFormat = liquidInstance.packedData & 0x3F;

                u8 posY = liquidInstance.packedOffset & 0xf;
                u8 posX = liquidInstance.packedOffset >> 4;

                u8 height = liquidInstance.packedSize & 0xf;
                u8 width = liquidInstance.packedSize >> 4;

                u32 vertexCount = (width + 1) * (height + 1);

                f32* heightMap = nullptr;
                if (hasVertexData)
                {
                    if (liquidVertexFormat == 0 || liquidVertexFormat == 1)
                    {
                        heightMap = reinterpret_cast<f32*>(&chunk.liquidvertexData[liquidDataOffset]);
                    }

                    u32 vertexDataBytes = 0;

                    // If LiquidVertexFormat == 0
                    vertexDataBytes += (vertexCount * sizeof(Terrain::LiquidVertexFormat_Height_Depth)) * (liquidVertexFormat == 0);

                    // If LiquidVertexFormat == 1
                    vertexDataBytes += (vertexCount * sizeof(Terrain::LiquidVertexFormat_Height_UV)) * (liquidVertexFormat == 1);

                    // If LiquidVertexFormat == 2
                    vertexDataBytes += (vertexCount * sizeof(Terrain::LiquidVertexFormat_Depth)) * (liquidVertexFormat == 2);

                    // If LiquidVertexFormat == 3
                    vertexDataBytes += (vertexCount * sizeof(Terrain::LiquidVertexFormat_Height_UV_Depth)) * (liquidVertexFormat == 3);

                    liquidDataOffset += vertexDataBytes;
                }

                for (u8 y = 0; y < height; y++)
                {
                    u32 yPlusOffset = y + posY;
                    f32 offsetZ = static_cast<f32>(yPlusOffset) * Terrain::MAP_PATCH_SIZE;

                    for (u8 x = 0; x < width; x++)
                    {
                        u32 xPlusOffset = x + posX;
                        f32 offsetX = static_cast<f32>(xPlusOffset) * Terrain::MAP_PATCH_SIZE;

                        u32 topLeftVert = x + (y * (width + 1));
                        u32 topRightVert = (topLeftVert + 1);
                        u32 bottomLeftVert = (topLeftVert + (width + 1));
                        u32 bottomRightVert = (bottomLeftVert + 1);
                        assert(topLeftVert < vertexCount);
                        assert(topRightVert < vertexCount);
                        assert(bottomLeftVert < vertexCount);
                        assert(bottomRightVert < vertexCount);

                        loadedWater.numInstances++;
                        ChunkToBeLoaded& chunkToBeLoaded = _chunksToBeLoaded.emplace_back();

                        f32 worldPosX = Terrain::MAP_HALF_SIZE - (chunkWorldPos.y + cellWorldPos.y + offsetX);
                        f32 worldPosZ = Terrain::MAP_HALF_SIZE - (chunkWorldPos.x + cellWorldPos.x + offsetZ);
                        vec3 pos = vec3(worldPosX, 0, worldPosZ);

                        f32 heightLevelMin = liquidInstance.heightLevel.x;
                        f32 heightLevelMax = liquidInstance.heightLevel.y;

                        f32 height = (heightLevelMin + heightLevelMax) / 2;

                        if (heightMap)
                        {
                            f32 topLeft = heightMap[topLeftVert];
                            f32 TopRight = heightMap[topRightVert];
                            f32 bottomLeft = heightMap[bottomLeftVert];
                            f32 bottomRight = heightMap[bottomRightVert];


                            chunkToBeLoaded.vertexHeightValues[0] = topLeft != 0 ? topLeft : height;
                            chunkToBeLoaded.vertexHeightValues[1] = TopRight != 0 ? TopRight : height;
                            chunkToBeLoaded.vertexHeightValues[2] = bottomLeft != 0 ? bottomLeft : height;
                            chunkToBeLoaded.vertexHeightValues[3] = bottomRight != 0 ? bottomRight : height;
                        }
                        else
                        {
                            chunkToBeLoaded.vertexHeightValues[0] = height;
                            chunkToBeLoaded.vertexHeightValues[1] = height;
                            chunkToBeLoaded.vertexHeightValues[2] = height;
                            chunkToBeLoaded.vertexHeightValues[3] = height;
                        }

                        //chunkToBeLoaded.vertexHeightValues[0] = heightMap ? heightMap[topLeftVert] : liquidInstance.heightLevel.x;
                        //chunkToBeLoaded.vertexHeightValues[1] = heightMap ? heightMap[topRightVert] : liquidInstance.heightLevel.x;
                        //chunkToBeLoaded.vertexHeightValues[2] = heightMap ? heightMap[bottomLeftVert] : liquidInstance.heightLevel.x;
                        //chunkToBeLoaded.vertexHeightValues[3] = heightMap ? heightMap[bottomRightVert] : liquidInstance.heightLevel.x;

                        if (heightMap)
                        {
                            //NC_LOG_MESSAGE("Case (i: %u, j: %u, y: %u, x: %u): Vertex(Count: %u, (%u, %u, %u, %u)) Height: (%f, %f, %f, %f)", i, j, y, x, vertexCount, topLeftVert, topRightVert, bottomLeftVert, bottomRightVert, heightMap[topLeftVert], heightMap[topRightVert], heightMap[bottomLeftVert], heightMap[bottomRightVert])
                        }

                        mat4x4 rotationMatrix = glm::eulerAngleXYZ(glm::radians(0.f), glm::radians(0.f), glm::radians(0.f));
                        chunkToBeLoaded.instanceMatrix = glm::translate(mat4x4(1.0f), pos) * rotationMatrix;
                    }
                }
            }

            liquidInfoOffset += numInstances;
        }
    }

    if (loadedWater.numInstances == 0)
        _loadedWater.clear();
    else
        NC_LOG_MESSAGE("Loaded %u Ocean Planes", loadedWater.numInstances);

    return true;
}

void WaterRenderer::ExecuteLoad()
{
    if (_loadedWater.size() == 0)
        return;
 
    LoadedWater& loadedWater = _loadedWater[0];
    
    // -- Create & Fill Instance Buffer
    {
        const size_t bufferSize = loadedWater.numInstances * sizeof(ChunkToBeLoaded);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "InstanceBuffer";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        loadedWater.instanceBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "InstanceBufferStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _chunksToBeLoaded.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(loadedWater.instanceBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    _chunksToBeLoaded.clear();
}