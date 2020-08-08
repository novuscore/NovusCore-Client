#include "TerrainRenderer.h"
#include <entt.hpp>
#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/MapSingleton.h"

#include <Renderer/Renderer.h>
#include <glm/gtc/matrix_transform.hpp>
#include <tracy/TracyVulkan.hpp>

const int WIDTH = 1920;
const int HEIGHT = 1080;

struct TerrainChunkData
{
    u32 alphaMapID = 0;
};

struct TerrainCellData
{
    u16 diffuseIDs[4] = { 0 };
};

TerrainRenderer::TerrainRenderer(Renderer::Renderer* renderer)
    : _renderer(renderer)
{
    CreatePermanentResources();
}

void TerrainRenderer::Update(f32 deltaTime)
{
    
}

void TerrainRenderer::AddTerrainDepthPrepass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
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
            TracySourceLocation(terrainDepth, "TerrainDepth", tracy::Color::Yellow2);
            commandList.BeginTrace(&terrainDepth);

            // Upload indices
            {
                Renderer::BufferDesc indexUploadBufferDesc;
                indexUploadBufferDesc.name = "TerrainCellIndexUploadBuffer";
                indexUploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
                indexUploadBufferDesc.size = sizeof(u16) * Terrain::NUM_INDICES_PER_CELL;
                indexUploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;

                Renderer::BufferID indexUploadBuffer = _renderer->CreateBuffer(indexUploadBufferDesc);

                void* indexBufferMemory = _renderer->MapBuffer(indexUploadBuffer);
                memcpy(indexBufferMemory, _indices.data(), indexUploadBufferDesc.size);
                _renderer->UnmapBuffer(indexUploadBuffer);
                commandList.CopyBuffer(_cellIndexBuffer, 0, indexUploadBuffer, 0, indexUploadBufferDesc.size);
            }

            // Upload instance data
            {
                const size_t cellCount = Terrain::MAP_CELLS_PER_CHUNK * (32 * 32);

                Renderer::BufferDesc uploadBufferDesc;
                uploadBufferDesc.name = "TerrainInstanceUploadBuffer";
                uploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
                uploadBufferDesc.size = sizeof(u32) * cellCount;
                uploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;

                Renderer::BufferID instanceUploadBuffer = _renderer->CreateBuffer(uploadBufferDesc);

                void* instanceBufferMemory = _renderer->MapBuffer(instanceUploadBuffer);
                u32* instanceData = static_cast<u32*>(instanceBufferMemory);
                u32 instanceDataIndex = 0;

                u32 minX = Math::Max(0, (32 - 16));
                u32 maxX = Math::Min(63, (32 + 16));

                u32 minY = Math::Max(0, (32 - 16));
                u32 maxY = Math::Min(63, (32 + 16));

                for (u32 y = minY; y < maxY; ++y)
                {
                    for (u32 x = minX; x < maxX; ++x)
                    {
                        const u32 chunkID = x + (y * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
                        for (u32 cellID = 0; cellID < Terrain::MAP_CELLS_PER_CHUNK; ++cellID)
                        {
                            instanceData[instanceDataIndex++] = (chunkID << 16) | (cellID & 0xffff);
                        }
                    }
                }
                assert(instanceDataIndex == cellCount);
                _renderer->UnmapBuffer(instanceUploadBuffer);
                commandList.CopyBuffer(_instanceBuffer, 0, instanceUploadBuffer, 0, uploadBufferDesc.size);
            }

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
            _passDescriptorSet.Bind("_chunkData"_h, _chunkBuffer);

            // Bind descriptorset
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);
            commandList.DrawIndexed(Terrain::NUM_INDICES_PER_CELL, Terrain::MAP_CELLS_PER_CHUNK * (32 * 32), 0, 0, 0);
            //commandList.DrawIndexedIndirect(_argumentBuffer, 0, 1 );

            commandList.EndPipeline(pipeline);
            commandList.EndTrace();
        });
    }
}

void TerrainRenderer::AddTerrainPass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex, u8 debugMode)
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
            TracySourceLocation(terrainPass, "TerrainPass", tracy::Color::Yellow2);
            commandList.BeginTrace(&terrainPass);

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
            pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_EQUAL;

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
            commandList.SetBuffer(0, _instanceBuffer);

            // Set index buffer
            commandList.SetIndexBuffer(_cellIndexBuffer, Renderer::IndexFormat::UInt16);

            // Bind viewbuffer
            _passDescriptorSet.Bind("ViewData"_h, viewConstantBuffer->GetBuffer(frameIndex));
            _passDescriptorSet.Bind("_vertexHeights"_h, _vertexBuffer);
            _passDescriptorSet.Bind("_cellData"_h, _cellBuffer);
            _passDescriptorSet.Bind("_chunkData"_h, _chunkBuffer);

            // Bind descriptorset
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);
            commandList.DrawIndexed(Terrain::NUM_INDICES_PER_CELL, Terrain::MAP_CELLS_PER_CHUNK * (32 * 32), 0, 0, 0);
            //commandList.DrawIndexedIndirect(_argumentBuffer, 0, 1);

            commandList.EndPipeline(pipeline);
            commandList.EndTrace();
        });
    }
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
    textureAlphaArrayDesc.size = 196; // Max 196 loaded chunks at a time (7 chunks draw radius, 14x14 chunks),

    _terrainAlphaTextureArray = _renderer->CreateTextureArray(textureAlphaArrayDesc);

    // Create and load a 1x1 pixel RGBA8 unorm texture with zero'ed data so we can use textureArray[0] as "invalid" textures, sampling it will return 0.0f on all channels
    Renderer::DataTextureDesc zeroAlphaTexture;
    zeroAlphaTexture.debugName = "TerrainZeroAlpha";
    zeroAlphaTexture.width = 1;
    zeroAlphaTexture.height = 1;
    zeroAlphaTexture.format = Renderer::IMAGE_FORMAT_R8G8B8A8_UNORM;
    zeroAlphaTexture.data = new u8[4]{ 0, 0, 0, 0 };

    u32 index;
    _renderer->CreateDataTextureIntoArray(zeroAlphaTexture, _terrainColorTextureArray, index);

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

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainCellIndexBuffer";
        desc.size = Terrain::NUM_INDICES_PER_CELL * sizeof(u16);
        desc.usage = Renderer::BUFFER_USAGE_INDEX_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _cellIndexBuffer = _renderer->CreateBuffer(desc);
    }

    {
        Renderer::BufferDesc desc;
        desc.name = "TerrainInstanceBuffer";
        desc.size = sizeof(u32) * Terrain::MAP_CELLS_PER_CHUNK * (Terrain::MAP_CHUNKS_PER_MAP_SIDE * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_VERTEX_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _instanceBuffer = _renderer->CreateBuffer(desc);
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

    _indices.resize(Terrain::NUM_INDICES_PER_CELL);

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
            _indices[indexIndex++] = centerVertex;
            _indices[indexIndex++] = topRightVertex;
            _indices[indexIndex++] = topLeftVertex;

            // Left triangle
            _indices[indexIndex++] = centerVertex;
            _indices[indexIndex++] = topLeftVertex;
            _indices[indexIndex++] = bottomLeftVertex;

            // Down triangle
            _indices[indexIndex++] = centerVertex;
            _indices[indexIndex++] = bottomLeftVertex;
            _indices[indexIndex++] = bottomRightVertex;

            // Right triangle
            _indices[indexIndex++] = centerVertex;
            _indices[indexIndex++] = bottomRightVertex;
            _indices[indexIndex++] = topRightVertex;
        }
    }

    // Load map
    Terrain::Map& map = mapSingleton.maps[0];
    LoadChunksAround(map, ivec2(32, 32), 16); // Goldshire
    //LoadChunksAround(map, ivec2(40, 32), 8); // Razor Hill
    //LoadChunksAround(map, ivec2(22, 25), 8); // Borean Tundra

    //LoadChunksAround(map, ivec2(0, 0), 8); // Goldshire
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

        TerrainCellData* cellData = static_cast<TerrainCellData*>(_renderer->MapBuffer(cellUploadBuffer));

        // Loop over all the cells in the chunk
        for (u32 i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
        {
            const Terrain::Cell& cell = chunk.cells[i];

            u8 layerCount = 0;
            for (auto layer : cell.layers)
            {
                if (layer.textureId == Terrain::LayerData::TextureIdInvalid)
                    break;

                const std::string& texturePath = stringTable.GetString(layer.textureId);

                Renderer::TextureDesc textureDesc;
                textureDesc.path = "Data/extracted/Textures/" + texturePath;

                u32 diffuseID;
                _renderer->LoadTextureIntoArray(textureDesc, _terrainColorTextureArray, diffuseID);

                cellData[i].diffuseIDs[layerCount++] = diffuseID;
            }

            //assert(cellData[i].diffuseIDs[0] != 0);
        }

        _renderer->UnmapBuffer(cellUploadBuffer);
        const u64 cellBufferOffset = (static_cast<u64>(chunkId) * Terrain::MAP_CELLS_PER_CHUNK) * sizeof(TerrainCellData);
        _renderer->CopyBuffer(_cellBuffer, cellBufferOffset, cellUploadBuffer, 0, cellDataUploadBufferDesc.size);
    }

    // ADTs store their alphamaps on a per-cell basis, one alphamap per used texture layer up to 4 different alphamaps
    // The different layers alphamaps can easily be combined into different channels of a single texture
    // But, that would still be 256 textures per chunk, which is a bit extreme
    // Instead we'll load our per-cell alphamaps and combine them into a single alphamap per chunk
    // This is gonna heavily decrease the amount of textures we need to use, and how big our texture arrays have to be

    // First we create our per-chunk alpha map descriptor
    Renderer::DataTextureDesc chunkAlphaMapDesc;
    chunkAlphaMapDesc.debugName = "ChunkAlphaMapArray";
    chunkAlphaMapDesc.width = 64;
    chunkAlphaMapDesc.height = 64;
    chunkAlphaMapDesc.layers = 256;
    chunkAlphaMapDesc.format = Renderer::ImageFormat::IMAGE_FORMAT_R8G8B8A8_UNORM;

    constexpr u32 numChannels = 4;
    const u32 chunkAlphaMapSize = chunkAlphaMapDesc.width * chunkAlphaMapDesc.height * chunkAlphaMapDesc.layers * numChannels; // 4 channels per pixel, 1 byte per channel
    chunkAlphaMapDesc.data = new u8[chunkAlphaMapSize]{ 0 }; // Allocate the data needed for it // TODO: Delete this...

    const u32 cellAlphaMapSize = 64 * 64; // This is the size of the per-cell alphamap
    for (u32 i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
    {
        const std::vector<Terrain::AlphaMap>& alphaMaps = chunk.alphaMaps[i];
        u32 numAlphaMaps = static_cast<u32>(alphaMaps.size());

        if (numAlphaMaps > 0)
        {
            for (u32 pixel = 0; pixel < cellAlphaMapSize; pixel++)
            {
                for (u32 channel = 0; channel < numAlphaMaps; channel++)
                {
                    u32 dst = (i * cellAlphaMapSize * numChannels) + (pixel * numChannels) + channel;
                    chunkAlphaMapDesc.data[dst] = alphaMaps[channel].alphaMap[pixel];
                }
            }
        }
    }

    // We have 4 uints per chunk for our diffuseIDs, this gives us a size and alignment of 16 bytes which is exactly what GPUs want
    // However, we need a fifth uint for alphaID, so we decided to pack it into the LAST diffuseID, which gets split into two uint16s
    // This is what it looks like
    // [1111] diffuseIDs[0]
    // [2222] diffuseIDs[1]
    // [3333] diffuseIDs[2]
    // [AA44] diffuseIDs[3] Alpha is read from the most significant bits, the fourth diffuseID read from the least 
    u32 alphaID;
    _renderer->CreateDataTextureIntoArray(chunkAlphaMapDesc, _terrainAlphaTextureArray, alphaID);
    
    // Upload chunk data.
    {
        Renderer::BufferDesc chunkDataUploadBufferDesc;
        chunkDataUploadBufferDesc.name = "TerrainChunkUploadBuffer";
        chunkDataUploadBufferDesc.size = sizeof(TerrainChunkData);
        chunkDataUploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;
        chunkDataUploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
        Renderer::BufferID chunkUploadBuffer = _renderer->CreateBuffer(chunkDataUploadBufferDesc);

        TerrainChunkData* chunkData = static_cast<TerrainChunkData*>(_renderer->MapBuffer(chunkUploadBuffer));
        chunkData->alphaMapID = alphaID;

        _renderer->UnmapBuffer(chunkUploadBuffer);
        const u64 chunkBufferOffset = static_cast<u64>(chunkId) * sizeof(TerrainChunkData);
        _renderer->CopyBuffer(_chunkBuffer, chunkBufferOffset, chunkUploadBuffer, 0, chunkDataUploadBufferDesc.size);
    }

    // Move the chunk to its proper position, this converts from ADT grid to world space, the axises don't line up, so the next two lines might be a bit confusing
    //f32 x = (-static_cast<f32>(chunkPosY) * Terrain::MAP_CHUNK_SIZE) + (Terrain::MAP_SIZE / 2.0f);
    //f32 z = (-static_cast<f32>(chunkPosX) * Terrain::MAP_CHUNK_SIZE) + (Terrain::MAP_SIZE / 2.0f); 

    // Create vertex and index (constant) buffers
    Renderer::BufferDesc vertexUploadBufferDesc;
    vertexUploadBufferDesc.name = "TerrainVertexUploadBuffer";
    vertexUploadBufferDesc.size = sizeof(f32) * Terrain::NUM_VERTICES_PER_CHUNK;
    vertexUploadBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;
    vertexUploadBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
    Renderer::BufferID vertexUploadBuffer = _renderer->CreateBuffer(vertexUploadBufferDesc);

    void* vertexBufferMemory = _renderer->MapBuffer(vertexUploadBuffer);
    for (size_t i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; ++i)
    {
        void* dstVertices = static_cast<u8*>(vertexBufferMemory) + (i * Terrain::CELL_TOTAL_GRID_SIZE * sizeof(f32));
        const void* srcVertices = chunk.cells[i].heightData;
        memcpy(dstVertices, srcVertices, Terrain::CELL_TOTAL_GRID_SIZE * sizeof(f32));
    }

    const u64 chunkVertexBufferOffset = chunkId * sizeof(f32) * Terrain::NUM_VERTICES_PER_CHUNK;
    _renderer->CopyBuffer(_vertexBuffer, chunkVertexBufferOffset, vertexUploadBuffer, 0, vertexUploadBufferDesc.size);
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
