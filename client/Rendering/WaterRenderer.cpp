#include "WaterRenderer.h"
#include "DebugRenderer.h"
#include "../Utils/ServiceLocator.h"
#include "../Utils/MapUtils.h"

#include <filesystem>
#include <GLFW/glfw3.h>
#include <InputManager.h>
#include <Renderer/Renderer.h>
#include <Renderer/RenderGraph.h>
#include <Utils/FileReader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <tracy/TracyVulkan.hpp>

#include "../ECS/Components/Singletons/MapSingleton.h"

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
    _constants.currentTime += deltaTime;
    /*static f32 updateTimer = 0;

    if (updateTimer >= 1 / 30.f)
    {
        if (++_textureIndex == 30)
        {
            _textureIndex = 0;
        }

        updateTimer -= 1 / 30.f;
    }
    else
        updateTimer += deltaTime;*/
}

void WaterRenderer::LoadWater(const std::vector<u16>& chunkIDs)
{
    RegisterChunksToBeLoaded(chunkIDs);
    ExecuteLoad();
}

void WaterRenderer::Clear()
{
    _vertices.clear();
    _indices.clear();

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
        data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::CLEAR);
        data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::CLEAR);

        return true; // Return true from setup to enable this pass, return false to disable it
    };

    const auto execute = [=](WaterPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList)
    {
        GPU_SCOPED_PROFILER_ZONE(commandList, WaterPass);

        Renderer::GraphicsPipelineDesc pipelineDesc;
        resources.InitializePipelineDesc(pipelineDesc);

        // Shaders
        Renderer::VertexShaderDesc vertexShaderDesc;
        vertexShaderDesc.path = "water.vs.hlsl";
        pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

        Renderer::PixelShaderDesc pixelShaderDesc;
        pixelShaderDesc.path = "water.ps.hlsl";
        pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

        // Depth state
        pipelineDesc.states.depthStencilState.depthEnable = true;
        pipelineDesc.states.depthStencilState.depthWriteEnable = true;
        pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::GREATER;

        // Rasterizer state
        pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::NONE; //Renderer::CullMode::BACK;
        pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

        // Blending state
        pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
        pipelineDesc.states.blendState.renderTargets[0].blendOp = Renderer::BlendOp::ADD;
        pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::SRC_ALPHA;
        pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::INV_SRC_ALPHA;

        // Render targets
        pipelineDesc.renderTargets[0] = data.mainColor;
        pipelineDesc.depthStencil = data.mainDepth;

        // Set pipeline
        Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
        commandList.BeginPipeline(pipeline);

        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

        commandList.PushMarker("Water", Color::White);

        _passDescriptorSet.Bind("_drawCallDatas", _drawCallDatasBuffer);
        _passDescriptorSet.Bind("_vertices", _vertexBuffer);
        _passDescriptorSet.Bind("_textures", _waterTextures);

        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

        {
            //commandList.PushConstant(&_constants, 0, sizeof(Constants));
            commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);

            u32 numDrawCalls = static_cast<u32>(_drawCalls.size());
            commandList.DrawIndexedIndirect(_drawCallsBuffer, 0, numDrawCalls);
        }

        commandList.PopMarker();

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
    samplerDesc.filter = Renderer::SamplerFilter::ANISOTROPIC;//Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.addressU = Renderer::TextureAddressMode::WRAP;
    samplerDesc.addressV = Renderer::TextureAddressMode::WRAP;
    samplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.shaderVisibility = Renderer::ShaderVisibility::PIXEL;
    samplerDesc.maxAnisotropy = 8;

    _sampler = _renderer->CreateSampler(samplerDesc);
    _passDescriptorSet.Bind("_sampler", _sampler);

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
    DebugHandler::Print("Loading Water");

    entt::registry* registry = ServiceLocator::GetGameRegistry();
    MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

    Terrain::Map& currentMap = mapSingleton.GetCurrentMap();

    for (const u16& chunkID : chunkIDs)
    {
        Terrain::Chunk& chunk = currentMap.chunks[chunkID];

        u16 chunkX = chunkID % Terrain::MAP_CHUNKS_PER_MAP_STRIDE;
        u16 chunkY = chunkID / Terrain::MAP_CHUNKS_PER_MAP_STRIDE;

        vec2 chunkPos = Terrain::MapUtils::GetChunkPosition(chunkID);

        vec3 chunkBasePos = Terrain::MAP_HALF_SIZE - vec3(Terrain::MAP_CHUNK_SIZE * chunkY, Terrain::MAP_CHUNK_SIZE * chunkX, Terrain::MAP_HALF_SIZE);

        if (chunk.liquidHeaders.size() == 0)
            continue;

        u32 liquidInfoOffset = 0;
        //u32 liquidDataOffset = 0;

        for (u32 i = 0; i < chunk.liquidHeaders.size(); i++)
        {
            Terrain::CellLiquidHeader& liquidHeader = chunk.liquidHeaders[i];

            bool hasAttributes = liquidHeader.attributesOffset > 0; // liquidHeader.packedData >> 7;
            u8 numInstances = liquidHeader.layerCount; // liquidHeader.packedData & 0x7F;

            if (numInstances == 0)
                continue;

            u16 cellX = i % Terrain::MAP_CELLS_PER_CHUNK_SIDE;
            u16 cellY = i / Terrain::MAP_CELLS_PER_CHUNK_SIDE;
            vec3 liquidBasePos = chunkBasePos - vec3(Terrain::MAP_CELL_SIZE * cellY, Terrain::MAP_CELL_SIZE * cellX, 0);

            const vec2 cellPos = Terrain::MapUtils::GetCellPosition(chunkPos, i);

            for (u32 j = 0; j < numInstances; j++)
            {
                Terrain::CellLiquidInstance& liquidInstance = chunk.liquidInstances[liquidInfoOffset + j];

                // Packed Format
                // Bit 1-6 (liquidVertexFormat)
                // Bit 7 (hasBitMaskForPatches)
                // Bit 8 (hasVertexData)

                bool hasVertexData = liquidInstance.vertexDataOffset > 0; // liquidInstance.packedData >> 7;
                bool hasBitMaskForPatches = liquidInstance.bitmapExistsOffset > 0; // (liquidInstance.packedData >> 6) & 0x1;
                u16 liquidVertexFormat = liquidInstance.liquidVertexFormat; // liquidInstance.packedData & 0x3F;

                u8 posY = liquidInstance.yOffset; //liquidInstance.packedOffset & 0xf;
                u8 posX = liquidInstance.xOffset; //liquidInstance.packedOffset >> 4;

                u8 height = liquidInstance.height; // liquidInstance.packedSize & 0xf;
                u8 width = liquidInstance.width; // liquidInstance.packedSize >> 4;

                u32 vertexCount = (width + 1) * (height + 1);

                f32* heightMap = nullptr;
                if (hasVertexData)
                {
                    if (liquidVertexFormat == 0 || liquidVertexFormat == 1 || liquidVertexFormat == 3)
                    {
                        heightMap = reinterpret_cast<f32*>(&chunk.liquidBytes[liquidInstance.vertexDataOffset]); //reinterpret_cast<f32*>(&chunk.liquidBytes[liquidDataOffset]);
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

                    //liquidDataOffset += vertexDataBytes;
                }

                DrawCall& drawCall = _drawCalls.emplace_back();
                DrawCallData& drawCallData = _drawCallDatas.emplace_back();

                drawCall.instanceCount = 1;
                drawCall.vertexOffset = static_cast<u32>(_vertices.size());
                drawCall.firstIndex = static_cast<u32>(_indices.size());
                drawCall.firstInstance = static_cast<u32>(_drawCalls.size()) - 1;

                // TODO: We should check if textureCount is always 30
                drawCallData.textureStartIndex = 0;
                drawCallData.textureCount = 30;

                for (u8 y = 0; y <= height; y++)
                {
                    // This represents World (Forward/Backward) in other words, our X axis
                    f32 offsetY = -(static_cast<f32>(posY + y) * Terrain::MAP_PATCH_SIZE);

                    for (u8 x = 0; x <= width; x++)
                    {
                        // This represents World (West/East) in other words, our Y axis
                        f32 offsetX = -(static_cast<f32>(posX + x) * Terrain::MAP_PATCH_SIZE);

                        //vec4 pos = vec4(cellPos.x + offsetX, cellPos.y + offsetY, liquidInstance.minHeightLevel, 1.0f); // vec4(cellPos.x + offsetX, cellPos.y + offsetY, liquidInstance.heightLevel.x, 1.0f);
                        vec4 pos = vec4(liquidBasePos, 1.0f) - vec4(Terrain::MAP_PATCH_SIZE * (y + liquidInstance.yOffset), Terrain::MAP_PATCH_SIZE * (x + liquidInstance.xOffset), liquidInstance.minHeightLevel, 0.0f);

                        if (heightMap && liquidInstance.liquidType != 2 && liquidVertexFormat != 2)
                        {
                            u32 vertexIndex = x + (y * (width + 1));
                            pos.z = heightMap[vertexIndex];
                        }

                        _vertices.push_back(pos);
                    }
                }

                for (u8 y = 0; y < height; y++)
                {
                    for (u8 x = 0; x < width; x++)
                    {
                        u16 topLeftVert = x + (y * (width + 1));
                        u16 topRightVert = topLeftVert + 1;
                        u16 bottomLeftVert = topLeftVert + (width + 1);
                        u16 bottomRightVert = bottomLeftVert + 1;

                        _indices.push_back(topLeftVert);
                        _indices.push_back(topRightVert);
                        _indices.push_back(bottomLeftVert);

                        _indices.push_back(topRightVert);
                        _indices.push_back(bottomRightVert);
                        _indices.push_back(bottomLeftVert);

                        drawCall.indexCount += 6;
                    }
                }
            }

            liquidInfoOffset += numInstances;
        }
    }

    DebugHandler::Print("Water: Loaded (%u, %u) Vertices/Indices", _vertices.size(), _indices.size());
    return true;
}

void WaterRenderer::ExecuteLoad()
{
    // -- Create DrawCall Buffer --
    {
        if (_drawCallsBuffer != Renderer::BufferID::Invalid())
            _renderer->QueueDestroyBuffer(_drawCallsBuffer);
    }
    {
        const size_t bufferSize = _drawCalls.size() * sizeof(DrawCall);

        Renderer::BufferDesc desc;
        desc.name = "WaterDrawCallBuffer";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::INDIRECT_ARGUMENT_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        _drawCallsBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "WaterDrawCallBufferStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer

        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _drawCalls.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_drawCallsBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create DrawCallDatas Buffer --
    {
        if (_drawCallDatasBuffer != Renderer::BufferID::Invalid())
            _renderer->QueueDestroyBuffer(_drawCallDatasBuffer);
    }
    {
        const size_t bufferSize = _drawCallDatas.size() * sizeof(DrawCallData);

        Renderer::BufferDesc desc;
        desc.name = "WaterDrawCallDatasBuffer";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        _drawCallDatasBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "WaterDrawCallDatasBufferStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer

        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _drawCallDatas.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_drawCallDatasBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create Vertex Buffer --
    {
        if (_vertexBuffer != Renderer::BufferID::Invalid())
            _renderer->QueueDestroyBuffer(_vertexBuffer);
    }
    {
        const size_t bufferSize = _vertices.size() * sizeof(vec4);

        Renderer::BufferDesc desc;
        desc.name = "VertexBuffer";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        _vertexBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "VertexBufferStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer

        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _vertices.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_vertexBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create Index Buffer --
    {
        if (_indexBuffer != Renderer::BufferID::Invalid())
            _renderer->QueueDestroyBuffer(_indexBuffer);
    }
    {
        const size_t bufferSize = _indices.size() * sizeof(u16);

        Renderer::BufferDesc desc;
        desc.name = "IndexBuffer";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::INDEX_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        _indexBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "IndexBufferStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer

        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _indices.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_indexBuffer, 0, stagingBuffer, 0, bufferSize);
    }
}