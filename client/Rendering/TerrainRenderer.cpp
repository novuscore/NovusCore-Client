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

    for (size_t i = 0; i < _chunkModelInstances.size(); i++)
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
            vertexShaderDesc.path = "Data/shaders/terrain.vert.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            // Constant buffers  TODO: Improve on this, if I set state 0 and 3 it won't work etc...
            pipelineDesc.states.constantBufferStates[0].enabled = true; // ViewCB
            pipelineDesc.states.constantBufferStates[0].shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_VERTEX;
            pipelineDesc.states.constantBufferStates[1].enabled = true; // ModelCB
            pipelineDesc.states.constantBufferStates[1].shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_VERTEX;

            // Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
            pipelineDesc.states.inputLayouts[0].enabled = true;
            pipelineDesc.states.inputLayouts[0].SetName("INSTANCEID");
            pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::INPUT_FORMAT_R32_UINT;
            pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_INSTANCE;

            // Viewport
            pipelineDesc.states.viewport.topLeftX = 0;
            pipelineDesc.states.viewport.topLeftY = 0;
            pipelineDesc.states.viewport.width = static_cast<f32>(WIDTH);
            pipelineDesc.states.viewport.height = static_cast<f32>(HEIGHT);
            pipelineDesc.states.viewport.minDepth = 0.0f;
            pipelineDesc.states.viewport.maxDepth = 1.0f;

            // ScissorRect
            pipelineDesc.states.scissorRect.left = 0;
            pipelineDesc.states.scissorRect.right = WIDTH;
            pipelineDesc.states.scissorRect.top = 0;
            pipelineDesc.states.scissorRect.bottom = HEIGHT;

            // Depth state
            pipelineDesc.states.depthStencilState.depthEnable = true;
            pipelineDesc.states.depthStencilState.depthWriteEnable = true;
            pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_LESS;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;
            pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

            // Samplers TODO: We don't care which samplers we have here, we just need the number of samplers
            pipelineDesc.states.samplers[0].enabled = true;

            pipelineDesc.depthStencil = data.mainDepth;

            // Set pipeline
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            // Set instance buffer
            commandList.SetBuffer(0, _terrainInstanceIDs->GetBuffer(frameIndex));

            // Set view constant buffer
            commandList.SetConstantBuffer(0, viewConstantBuffer->GetDescriptor(frameIndex), frameIndex);

            // Render main layer
            Renderer::RenderLayer& mainLayer = _renderer->GetRenderLayer("Terrain"_h);

            for (auto const& model : mainLayer.GetModels())
            {
                auto const& instances = model.second;

                for (auto const& instance : instances)
                {
                    // Set model constant buffer
                    commandList.SetConstantBuffer(1, instance->GetDescriptor(frameIndex), frameIndex);

                    TerrainInstanceData* terrainInstanceData = instance->GetOptional<TerrainInstanceData>();

                    // Set vertex storage buffer
                    commandList.SetStorageBuffer(2, terrainInstanceData->vertexBuffer->GetDescriptor(frameIndex), frameIndex);

                    // Draw
                    commandList.DrawIndexedBindless(_chunkModel, Terrain::NUM_INDICES_PER_CHUNK, Terrain::MAP_CELLS_PER_CHUNK);
                }
            }
            commandList.EndPipeline(pipeline);
        });
    }
}

void TerrainRenderer::AddTerrainPass(Renderer::RenderGraph* renderGraph, Renderer::ConstantBuffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
    // Terrain Pass
    {
        struct TerrainPassData
        {
            Renderer::RenderPassMutableResource mainColor;
            Renderer::RenderPassMutableResource mainDepth;
            Renderer::RenderPassResource terrainTexture;
        };

        renderGraph->AddPass<TerrainPassData>("Terrain Pass",
            [=](TerrainPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
            data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
            data.terrainTexture = builder.Read(_terrainTexture, Renderer::RenderGraphBuilder::ShaderStage::SHADER_STAGE_PIXEL);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
            [=, &renderGraph](TerrainPassData& data, Renderer::CommandList& commandList) // Execute
        {
            Renderer::GraphicsPipelineDesc pipelineDesc;
            renderGraph->InitializePipelineDesc(pipelineDesc);

            // Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/terrain.vert.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "Data/shaders/terrain.frag.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            // Constant buffers  TODO: Improve on this, if I set state 0 and 3 it won't work etc...
            pipelineDesc.states.constantBufferStates[0].enabled = true; // ViewCB
            pipelineDesc.states.constantBufferStates[0].shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_VERTEX;
            pipelineDesc.states.constantBufferStates[1].enabled = true; // ModelCB
            pipelineDesc.states.constantBufferStates[1].shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_VERTEX;

            // Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
            pipelineDesc.states.inputLayouts[0].enabled = true;
            pipelineDesc.states.inputLayouts[0].SetName("INSTANCEID");
            pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::INPUT_FORMAT_R32_UINT;
            pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_INSTANCE;

            // Viewport
            pipelineDesc.states.viewport.topLeftX = 0;
            pipelineDesc.states.viewport.topLeftY = 0;
            pipelineDesc.states.viewport.width = static_cast<f32>(WIDTH);
            pipelineDesc.states.viewport.height = static_cast<f32>(HEIGHT);
            pipelineDesc.states.viewport.minDepth = 0.0f;
            pipelineDesc.states.viewport.maxDepth = 1.0f;

            // ScissorRect
            pipelineDesc.states.scissorRect.left = 0;
            pipelineDesc.states.scissorRect.right = WIDTH;
            pipelineDesc.states.scissorRect.top = 0;
            pipelineDesc.states.scissorRect.bottom = HEIGHT;

            // Depth state
            pipelineDesc.states.depthStencilState.depthEnable = true;
            pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_EQUAL;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;
            pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

            // Samplers TODO: We don't care which samplers we have here, we just need the number of samplers
            pipelineDesc.states.samplers[0].enabled = true;

            // Textures TODO: We don't care which textures we have here, we just need the number of textures
            pipelineDesc.textures[0] = data.terrainTexture;

            // Render targets
            pipelineDesc.renderTargets[0] = data.mainColor;

            pipelineDesc.depthStencil = data.mainDepth;

            // Set pipeline
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            // Set instance buffer
            commandList.SetBuffer(0, _terrainInstanceIDs->GetBuffer(frameIndex));

            // Set view constant buffer
            commandList.SetConstantBuffer(0, viewConstantBuffer->GetDescriptor(frameIndex), frameIndex);

            // Set sampler and texture
            commandList.SetSampler(3, _linearSampler);
            //commandList.SetTexture(3, _terrainTexture);

            // Render main layer
            Renderer::RenderLayer& mainLayer = _renderer->GetRenderLayer("Terrain"_h);

            for (auto const& model : mainLayer.GetModels())
            {
                auto const& instances = model.second;

                for (auto const& instance : instances)
                {
                    // Set model constant buffer
                    commandList.SetConstantBuffer(1, instance->GetDescriptor(frameIndex), frameIndex);

                    TerrainInstanceData* terrainInstanceData = instance->GetOptional<TerrainInstanceData>();

                    // Set vertex storage buffer
                    commandList.SetStorageBuffer(2, terrainInstanceData->vertexBuffer->GetDescriptor(frameIndex), frameIndex);
                    
                    // Set texture array
                    commandList.SetTextureArray(4, terrainInstanceData->textureArray);

                    // Set constant buffer
                    commandList.SetConstantBuffer(5, terrainInstanceData->chunkData->GetDescriptor(frameIndex), frameIndex);

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

    // Load map
    Terrain::Map& map = mapSingleton.maps[0];
    LoadChunksAround(map, ivec2(31, 49), 5); // Goldshire
    //LoadChunksAround(map, ivec2(40, 32), 5); // Razor Hill

    // Create texture array TODO: REMOVE
    Renderer::TextureArrayDesc textureSetDesc;
    textureSetDesc.size = 4096;

    _terrainTextureArray = _renderer->CreateTextureArray(textureSetDesc);

    // Create texture inside of texture set
    Renderer::TextureDesc textureDesc;
    textureDesc.path = "Data/textures/debug.dds";

    u32 arrayIndex;
    _terrainTexture = _renderer->LoadTextureIntoArray(textureDesc, _terrainTextureArray, arrayIndex);

    // Sampler
    Renderer::SamplerDesc samplerDesc;
    samplerDesc.enabled = true;
    samplerDesc.filter = Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.addressU = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressV = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressW = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_PIXEL;

    _linearSampler = _renderer->CreateSampler(samplerDesc);

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

    // Create texture array
    // TODO OPTIMIZATION: We currently create one texture array per chunk, but in theory we could have one texture array total.
    // We're blocked from doing this due to two reasons:
    // 1. Each cell currently has a 64x64 alphamap texture, that's 256 textures per chunk which would quickly fill our array 
    //    (we have a self imposed cap of 4096 textures, our target hardware has a cap of 8096 descriptors in one array so we can increase that a bit if necessary)
    Renderer::TextureArrayDesc textureArrayDesc;
    textureArrayDesc.size = 4096;//static_cast<u32>(textureIds.size()) + numAlphaMaps;

    terrainInstanceData->textureArray = _renderer->CreateTextureArray(textureArrayDesc);
    
    // Create and load a 1x1 pixel RGBA8 unorm texture with zero'ed data so we can use textureArray[0] as "invalid" textures, sampling it will return 0.0f on all channels
    Renderer::DataTextureDesc zeroAlphaTexture;
    zeroAlphaTexture.debugName = "TerrainZeroAlpha";
    zeroAlphaTexture.width = 1;
    zeroAlphaTexture.height = 1;
    zeroAlphaTexture.format = Renderer::IMAGE_FORMAT_R8G8B8A8_UNORM;
    zeroAlphaTexture.data = new u8[4]{ 0 };

    u32 index;
    _renderer->CreateDataTextureIntoArray(zeroAlphaTexture, terrainInstanceData->textureArray, index);

    // Get the vertices, indices and textureIDs of the chunk
    std::vector<TerrainVertex> chunkVertices;
    const size_t numVertices = Terrain::NUM_VERTICES_PER_CHUNK;
    chunkVertices.resize(numVertices);

    std::vector<u32> textureIds;
    u32 numAlphaMaps = 0;

    StringTable& stringTable = map.stringTables[chunkId];

    // Loop over all the cells in the chunk
    for (int i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
    {
        Terrain::Cell& cell = chunk.cells[i];

        u8 layerCount = 0;
        for (auto layer : cell.layers)
        {
            if (layer.textureId != Terrain::LayerData::TextureIdInvalid)
            {
                const std::string& texturePath = stringTable.GetString(layer.textureId);

                Renderer::TextureDesc textureDesc;
                textureDesc.path = "Data/extracted/Textures/" + texturePath;

                u32 diffuseID;
                _renderer->LoadTextureIntoArray(textureDesc, terrainInstanceData->textureArray, diffuseID);
                assert(diffuseID < 65536); // Because of the way we pack diffuseIDs[3] and alphaID, this should never be bigger than a u16, see where we create the alpha texture below

                terrainInstanceData->chunkData->resource[i].diffuseIDs[layerCount] = diffuseID;
            }
            
            layerCount++;
        }

        const f32 cellPosX = -static_cast<f32>(i % Terrain::MAP_CELLS_PER_CHUNK_SIDE) * Terrain::CELL_SIZE;
        const f32 cellPosZ = static_cast<f32>(i / Terrain::MAP_CELLS_PER_CHUNK_SIDE) * Terrain::CELL_SIZE;

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

                f32 u = static_cast<f32>(vertex % 17);
                f32 v = Math::Floor(static_cast<f32>(vertex) / 17.0f);

                // Handle UV offsets for the inner array
                if (u > 8.01f) 
                {
                    v = v + 0.5f;
                    u = u - 8.5f;
                }

                chunkVertices[vertex + cellOffset].texCoord = vec4(u, v, 0.0f, 0.0f);//vec2(u, v);
                //chunkVertices[vertex + cellOffset].vertex.normal = vec3(0.0f, 1.0f, 0.0f); // TODO: Actual normals for  terrain

                vertex++;
            }
        }
    }

    // Load all alpha maps into the texture array
    for (int i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
    {
        std::vector<Terrain::AlphaMap>& alphaMaps = chunk.alphaMaps[i];
        u32 numAlphaMaps = static_cast<u32>(alphaMaps.size());

        if (numAlphaMaps == 0)
        {
            continue;
        }

        // TODO OPTIMIZATION: We currently have 1 alphamap texture per cell, this means that each chunk has 256 alphamap textures.
        // These textures are 64x64 pixels, so we could merge them all into one texture per chunk, which would be 1024x1024
        // The drawback here is that we'll need to figure out how to UV into it, and the 1024x1024 texture will need all channels for alpha even when some cells don't need that many channels
        Renderer::DataTextureDesc alphaMapDesc;
        alphaMapDesc.debugName = "CellAlphaMap";

        u32 numChannels = numAlphaMaps;

        switch (numAlphaMaps)
        {
        case 1:
            alphaMapDesc.format = Renderer::ImageFormat::IMAGE_FORMAT_R8_UNORM;
            break;
        case 2:
            alphaMapDesc.format = Renderer::ImageFormat::IMAGE_FORMAT_R8G8_UNORM;
            break;
        case 3:
            alphaMapDesc.format = Renderer::ImageFormat::IMAGE_FORMAT_R8G8B8A8_UNORM;
            numChannels = 4;
            break;
        case 4:
            alphaMapDesc.format = Renderer::ImageFormat::IMAGE_FORMAT_R8G8B8A8_UNORM;
            break;

        default:
            NC_LOG_FATAL("A cell had more than 4 alphamapped layers, this should never be possible?")
        }

        alphaMapDesc.width = 64;
        alphaMapDesc.height = 64;

        alphaMapDesc.data = new u8[4096 * numChannels]{ 0 };

        for (u32 pixel = 0; pixel < 4096; pixel++)
        {
            for (u32 channel = 0; channel < numAlphaMaps; channel++)
            {
                u32 dst = (pixel * numChannels) + (channel);
                alphaMapDesc.data[dst] = alphaMaps[channel].alphaMap[pixel];
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
        _renderer->CreateDataTextureIntoArray(alphaMapDesc, terrainInstanceData->textureArray, alphaID);
        assert(alphaID < 65536); // Because of the way we pack diffuseIDs[3] and alphaID, this should never be bigger than a u16

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

void TerrainRenderer::LoadChunksAround(Terrain::Map& map, ivec2 middleChunk, u16 radius)
{
    ivec2 startPos = ivec2(middleChunk.x - radius, middleChunk.y - radius);
    ivec2 endPos = ivec2(middleChunk.x + radius, middleChunk.y + radius);

    for(i32 y = startPos.y; y < endPos.y; y++)
    {
        for (i32 x = startPos.x; x < endPos.x; x++)
        {
            LoadChunk(map, x, y);
        }
    }
}
