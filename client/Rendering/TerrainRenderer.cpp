#include "TerrainRenderer.h"
#include <entt.hpp>
#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/MapSingleton.h"

#include <Renderer/Renderer.h>
#include <glm/gtc/matrix_transform.hpp>

const int WIDTH = 1920;
const int HEIGHT = 1080;

TerrainRenderer::TerrainRenderer(Renderer::Renderer* renderer)
    : _renderer(renderer)
{
    CreatePermanentResources();
}

void TerrainRenderer::Update(f32 deltaTime)
{
    Renderer::RenderLayer& terrainLayer = _renderer->GetRenderLayer("Terrain"_h);
    terrainLayer.Reset();

    u32 numInstances = static_cast<u32>(_chunkModelInstances.size());
    for (size_t i = 0; i < numInstances; i++)
    {
        terrainLayer.RegisterModel(Renderer::ModelID::Invalid(), &_chunkModelInstances[i]);
    }
}

void TerrainRenderer::AddTerrainDepthPrepass(Renderer::RenderGraph* renderGraph, Renderer::ConstantBuffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::DepthImageID depthTarget, u8 frameIndex)
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
            [=, &renderGraph](TerrainDepthPrepassData& data, Renderer::CommandList& commandList) // Execute
        {
            Renderer::GraphicsPipelineDesc pipelineDesc;
            renderGraph->InitializePipelineDesc(pipelineDesc);

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
            commandList.SetBuffer(0, _terrainInstanceIDs->GetBuffer(frameIndex));

            // Bind viewbuffer
            _passDescriptorSet.Bind("ViewData"_h, viewConstantBuffer);

            // Bind descriptorset
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            // Render main layer
            Renderer::RenderLayer& mainLayer = _renderer->GetRenderLayer("Terrain"_h);

            for (auto const& model : mainLayer.GetModels())
            {
                auto const& instances = model.second;

                for (auto const& instance : instances)
                {
                    TerrainInstanceData* terrainInstanceData = instance->GetOptional<TerrainInstanceData>();

                    _drawDescriptorSet.Bind("ModelData"_h, instance->GetConstantBuffer());
                    _drawDescriptorSet.Bind("_vertexData"_h, terrainInstanceData->vertexBuffer);
                    _drawDescriptorSet.Bind("ChunkData"_h, terrainInstanceData->chunkData);

                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawDescriptorSet, frameIndex);

                    // Draw
                    commandList.DrawIndexedBindless(_chunkModel, Terrain::NUM_INDICES_PER_CHUNK, Terrain::MAP_CELLS_PER_CHUNK);
                }
            }
            commandList.EndPipeline(pipeline);
        });
    }
}

void TerrainRenderer::AddTerrainPass(Renderer::RenderGraph* renderGraph, Renderer::ConstantBuffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex, u8 debugMode)
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
            [=, &renderGraph](TerrainPassData& data, Renderer::CommandList& commandList) // Execute
        {
            Renderer::GraphicsPipelineDesc pipelineDesc;
            renderGraph->InitializePipelineDesc(pipelineDesc);

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
            commandList.SetBuffer(0, _terrainInstanceIDs->GetBuffer(frameIndex));

            // Update debug data buffer
            _terrainDebugData->resource.debugMode = debugMode;
            _terrainDebugData->Apply(frameIndex);

            // Bind viewbuffer
            _passDescriptorSet.Bind("ViewData"_h, viewConstantBuffer);

            // Bind descriptorset
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            // Render main layer
            Renderer::RenderLayer& mainLayer = _renderer->GetRenderLayer("Terrain"_h);

            for (auto const& model : mainLayer.GetModels())
            {
                auto const& instances = model.second;

                for (auto const& instance : instances)
                {
                    TerrainInstanceData* terrainInstanceData = instance->GetOptional<TerrainInstanceData>();

                    _drawDescriptorSet.Bind("ModelData"_h, instance->GetConstantBuffer());
                    _drawDescriptorSet.Bind("_vertexData"_h, terrainInstanceData->vertexBuffer);
                    _drawDescriptorSet.Bind("ChunkData"_h, terrainInstanceData->chunkData);

                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawDescriptorSet, frameIndex);

                    // Draw
                    commandList.DrawIndexedBindless(_chunkModel, Terrain::NUM_INDICES_PER_CHUNK, Terrain::MAP_CELLS_PER_CHUNK);
                }
            }
            commandList.EndPipeline(pipeline);
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
    zeroAlphaTexture.data = new u8[4]{ 0 };

    u32 index;
    _renderer->CreateDataTextureIntoArray(zeroAlphaTexture, _terrainColorTextureArray, index);

    // Load map
    Terrain::Map& map = mapSingleton.maps[0];
    LoadChunksAround(map, ivec2(31, 49), 8); // Goldshire
    //LoadChunksAround(map, ivec2(40, 32), 8); // Razor Hill
    //LoadChunksAround(map, ivec2(22, 25), 8); // Borean Tundra

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

    // Debug data constant buffer
    _terrainDebugData = _renderer->CreateConstantBuffer<TerrainDebugData>();
    _terrainDebugData->ApplyAll();

    // Descriptor sets
    _passDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _passDescriptorSet.Bind("_alphaSampler"_h, _alphaSampler);
    _passDescriptorSet.Bind("_colorSampler"_h, _colorSampler);
    _passDescriptorSet.Bind("_terrainColorTextures"_h, _terrainColorTextureArray);
    _passDescriptorSet.Bind("_terrainAlphaTextures"_h, _terrainAlphaTextureArray);
    _passDescriptorSet.Bind("DebugData"_h, _terrainDebugData);

    _drawDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());

    // Create a chunk model with no vertices but the correct indices
    Renderer::PrimitiveModelDesc modelDesc;
    modelDesc.debugName = "TerrainChunk";
    modelDesc.indices.reserve(Terrain::NUM_INDICES_PER_CHUNK);

    // Fill index buffer
    for (i32 row = 0; row < Terrain::CELL_INNER_GRID_SIDE; row++)
    {
        for (i32 col = 0; col < Terrain::CELL_INNER_GRID_SIDE; col++)
        {
            i32 baseVertex = (row * Terrain::CELL_TOTAL_GRID_SIDE + col);

            //1     2
            //   0
            //3     4

            i32 topLeftVertex = baseVertex;
            i32 topRightVertex = baseVertex + 1;
            i32 bottomLeftVertex = baseVertex + Terrain::CELL_TOTAL_GRID_SIDE;
            i32 bottomRightVertex = baseVertex + Terrain::CELL_TOTAL_GRID_SIDE + 1;
            i32 centerVertex = baseVertex + Terrain::CELL_OUTER_GRID_SIDE;

            // Up triangle
            modelDesc.indices.push_back(centerVertex);
            modelDesc.indices.push_back(topRightVertex);
            modelDesc.indices.push_back(topLeftVertex);

            // Left triangle
            modelDesc.indices.push_back(centerVertex);
            modelDesc.indices.push_back(topLeftVertex);
            modelDesc.indices.push_back(bottomLeftVertex);

            // Down triangle
            modelDesc.indices.push_back(centerVertex);
            modelDesc.indices.push_back(bottomLeftVertex);
            modelDesc.indices.push_back(bottomRightVertex);

            // Right triangle
            modelDesc.indices.push_back(centerVertex);
            modelDesc.indices.push_back(bottomRightVertex);
            modelDesc.indices.push_back(topRightVertex);
        }
    }

    _chunkModel = _renderer->CreatePrimitiveModel(modelDesc);

    // Initialize the instance IDs to go from 0 to Terrain::MAP_CELLS_PER_CHUNK
    _terrainInstanceIDs = _renderer->CreateConstantBuffer<std::array<u32, Terrain::MAP_CELLS_PER_CHUNK>>();
    for (u32 i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
    {
        _terrainInstanceIDs->resource[i] = i;
    }
    _terrainInstanceIDs->ApplyAll();
}

void TerrainRenderer::LoadChunk(Terrain::Map& map, u16 chunkPosX, u16 chunkPosY)
{
    u16 chunkId;
    map.GetChunkIdFromChunkPosition(chunkPosX, chunkPosY, chunkId);

    Terrain::Chunk& chunk = map.chunks[chunkId];

    // Create one chunk instance per chunk
    Renderer::InstanceData chunkInstance;
    chunkInstance.Init(_renderer);

    // Create terrain instance data
    TerrainInstanceData* terrainInstanceData = new TerrainInstanceData();
    chunkInstance.SetOptional(terrainInstanceData);

    terrainInstanceData->chunkData = _renderer->CreateConstantBuffer<std::array<TerrainChunkData, Terrain::MAP_CELLS_PER_CHUNK>>();

    // Get the vertices, indices and textureIDs of the chunk
    std::vector<TerrainVertex> chunkVertices;
    const size_t numVertices = Terrain::NUM_VERTICES_PER_CHUNK;
    chunkVertices.resize(numVertices);

    StringTable& stringTable = map.stringTables[chunkId];

    // Loop over all the cells in the chunk
    for (u32 i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
    {
        Terrain::Cell& cell = chunk.cells[i];

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
            assert(diffuseID < 65536); // Because of the way we pack diffuseIDs[3] and alphaID, this should never be bigger than a u16, see where we create the alpha texture below

            terrainInstanceData->chunkData->resource[i].diffuseIDs[layerCount++] = diffuseID;
        }

        const u32 cellX = i % Terrain::MAP_CELLS_PER_CHUNK_SIDE;
        const u32 cellY = i / Terrain::MAP_CELLS_PER_CHUNK_SIDE;

        const f32 cellPosX = -static_cast<f32>(cellX) * Terrain::CELL_SIZE;
        const f32 cellPosZ = static_cast<f32>(cellY) * Terrain::CELL_SIZE;

        // Vertices
        const size_t cellOffset = i * Terrain::CELL_TOTAL_GRID_SIZE;
        size_t vertex = 0;
        for (size_t row = 0; row < Terrain::CELL_TOTAL_GRID_SIDE; row++)
        {
            bool isEvenRow = (row % 2) == 0;
            size_t rowLength = isEvenRow ? Terrain::CELL_OUTER_GRID_SIDE : Terrain::CELL_INNER_GRID_SIDE;
            f32 offset = isEvenRow ? 0.0f : 0.5f;

            for (size_t col = 0; col < rowLength; col++)
            {
                f32 vertexPosX = -(((static_cast<f32>(col) + offset) / 8.0f) * Terrain::CELL_SIZE) - (Terrain::CELL_SIZE / 2.0f);
                f32 vertexPosY = cell.heightData[vertex];
                f32 vertexPosZ = (((static_cast<f32>(row) * 0.5f) / 8.0f) * Terrain::CELL_SIZE) - (Terrain::CELL_SIZE / 2.0f);

                chunkVertices[vertex + cellOffset].position = vec4(vertexPosX + cellPosX, vertexPosY, vertexPosZ + cellPosZ, 0.0f);

                vec2 uv = vec2(static_cast<f32>(vertex % 17), Math::Floor(static_cast<f32>(vertex) / 17.0f));

                // Handle UV offsets for the inner array
                if (uv.x > 8.01f)
                {
                    uv.x = uv.x - 8.5f;
                    uv.y = uv.y + 0.5f;
                }

                chunkVertices[vertex + cellOffset].texCoord = vec4(uv, 0.0f, 0.0f);
                //chunkVertices[vertex + cellOffset].vertex.normal = vec3(0.0f, 1.0f, 0.0f); // TODO: Actual normals for  terrain

                vertex++;
            }
        }
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
        std::vector<Terrain::AlphaMap>& alphaMaps = chunk.alphaMaps[i];
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
    assert(alphaID < 65536); // Because of the way we pack diffuseIDs[3] and alphaID, this should never be bigger than a u16

    // TODO: alphaID is only needed on a per-chunk basis, not per-cell, so it should not be inside of chunkData I believe
    for (u32 i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
    {
        // This line packs alphaID into the most significant bits of diffuseIDs[3]
        terrainInstanceData->chunkData->resource[i].diffuseIDs[3] = (alphaID << 16) | terrainInstanceData->chunkData->resource[i].diffuseIDs[3];
    }

    // Move the chunk to its proper position, this converts from ADT grid to world space, the axises don't line up, so the next two lines might be a bit confusing
    f32 x = (-static_cast<f32>(chunkPosY) * Terrain::MAP_CHUNK_SIZE) + (Terrain::MAP_SIZE / 2.0f);
    f32 z = (-static_cast<f32>(chunkPosX) * Terrain::MAP_CHUNK_SIZE) + (Terrain::MAP_SIZE / 2.0f); 

    vec3 chunkPosition = vec3(x, 0.0f, z);
    const mat4x4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
    const mat4x4 translationMatrix = glm::translate(glm::mat4(1.0f), chunkPosition);
    chunkInstance.modelMatrix = translationMatrix * rotationMatrix;

    // Create vertex and index (constant) buffers
    terrainInstanceData->vertexBuffer = _renderer->CreateStorageBuffer<std::array<TerrainVertex, Terrain::NUM_VERTICES_PER_CHUNK>>();

    // Set vertex and index buffers to the vectors we created above
    memcpy(terrainInstanceData->vertexBuffer->resource.data(), chunkVertices.data(), chunkVertices.size() * sizeof(TerrainVertex));

    // Apply buffers
    chunkInstance.ApplyAll();
    terrainInstanceData->vertexBuffer->ApplyAll();
    terrainInstanceData->chunkData->ApplyAll();
    
    _chunkModelInstances.push_back(chunkInstance);
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
