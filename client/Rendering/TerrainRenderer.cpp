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
    std::fill(std::begin(_chunkModels), std::end(_chunkModels), Renderer::ModelID::Invalid());
    CreatePermanentResources();
}

void TerrainRenderer::Update(f32 deltaTime)
{
    Renderer::RenderLayer& depthPrepassLayer = _renderer->GetRenderLayer("DepthPrepass"_h);
    Renderer::RenderLayer& terrainLayer = _renderer->GetRenderLayer("Terrain"_h);
    terrainLayer.Reset();

    for (size_t i = 0; i < _chunkModels.size(); i++)
    {
        depthPrepassLayer.RegisterModel(_chunkModels[i], &_chunkModelInstances[i]);
        terrainLayer.RegisterModel(_chunkModels[i], &_chunkModelInstances[i]);
    }
}

void TerrainRenderer::AddTerrainPass(Renderer::RenderGraph* renderGraph, Renderer::ConstantBuffer<ViewConstantBuffer>& viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
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
            [&](TerrainPassData& data, Renderer::CommandList& commandList) // Execute
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
            pipelineDesc.states.inputLayouts[0].SetName("POSITION");
            pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::INPUT_FORMAT_R32G32B32_FLOAT;
            pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
            pipelineDesc.states.inputLayouts[1].enabled = true;
            pipelineDesc.states.inputLayouts[1].SetName("NORMAL");
            pipelineDesc.states.inputLayouts[1].format = Renderer::InputFormat::INPUT_FORMAT_R32G32B32_FLOAT;
            pipelineDesc.states.inputLayouts[1].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
            pipelineDesc.states.inputLayouts[2].enabled = true;
            pipelineDesc.states.inputLayouts[2].SetName("TEXCOORD");
            pipelineDesc.states.inputLayouts[2].format = Renderer::InputFormat::INPUT_FORMAT_R32G32_FLOAT;
            pipelineDesc.states.inputLayouts[2].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;

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

            // Set view constant buffer
            commandList.SetConstantBuffer(0, viewConstantBuffer.GetGPUResource(frameIndex), frameIndex);

            // Set sampler and texture
            commandList.SetSampler(2, _linearSampler);
            commandList.SetTextureArray(3, _terrainTextureArray);
            //commandList.SetTexture(3, _terrainTexture);

            // Render main layer
            Renderer::RenderLayer& mainLayer = _renderer->GetRenderLayer("Terrain"_h);

            for (auto const& model : mainLayer.GetModels())
            {
                auto const& modelID = Renderer::ModelID(model.first);
                auto const& instances = model.second;

                for (auto const& instance : instances)
                {
                    // Set model constant buffer
                    commandList.SetConstantBuffer(1, instance->GetGPUResource(frameIndex), frameIndex);

                    // Draw
                    commandList.DrawInstanced(modelID, Terrain::MAP_CELLS_PER_CHUNK);
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

    // Load map 0, chunk 31 49, it should be Goldshire
    Terrain::Map& map = mapSingleton.maps[0];
    LoadChunksAround(map, ivec2(31, 49), 5);

    // Create texture array
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
}

void TerrainRenderer::LoadChunk(Terrain::Map& map, u16 chunkPosX, u16 chunkPosY)
{
    u16 chunkId;
    map.GetChunkIdFromChunkPosition(chunkPosX, chunkPosY, chunkId);

    Terrain::Chunk& chunk = map.chunks[chunkId];

    char str[50];
    StringUtils::FormatString(str, sizeof(str), "Chunk %u (%u, %u)", chunkId, chunkPosX, chunkPosY);

    // Construct one primitive model containing all the cells of the chunk
    Renderer::PrimitiveModelDesc chunkModelDesc;
    chunkModelDesc.debugName = str;

    std::vector<Renderer::Vertex>& chunkVertices = chunkModelDesc.vertices;
    const size_t numVertices = Terrain::MAP_CELLS_PER_CHUNK * Terrain::CELL_TOTAL_GRID_SIZE;
    chunkVertices.resize(numVertices);

    std::vector<u32>& chunkIndices = chunkModelDesc.indices;
    const size_t numIndices = Terrain::CELL_TOTAL_GRID_SIZE * 4 * 3;
    chunkIndices.reserve(numIndices);

    std::vector<u32> textureIds;

    // Loop over all the cells in the chunk
    for (int i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
    {
        Terrain::Cell& cell = chunk.cells[i];

        for (auto layer : cell.layers)
        {
            if (layer.textureId == Terrain::LayerData::TextureIdInvalid)
            {
                break;
            }

            if (std::find(textureIds.begin(), textureIds.end(), layer.textureId) == textureIds.end())
            {
                textureIds.push_back(layer.textureId);
            }
        }

        const f32 cellPosX = static_cast<f32>(i % Terrain::MAP_CELLS_PER_CHUNK_SIDE) * Terrain::CELL_SIZE;
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
                f32 vertexPosX = (((static_cast<f32>(col) + offset) / 8.0f) * Terrain::CELL_SIZE) - (Terrain::CELL_SIZE / 2.0f);
                f32 vertexPosY = cell.heightData[vertex];
                f32 vertexPosZ = (((static_cast<f32>(row) * 0.5f) / 8.0f) * Terrain::CELL_SIZE) - (Terrain::CELL_SIZE / 2.0f);

                chunkVertices[vertex + cellOffset].pos = vec3(vertexPosX + cellPosX, vertexPosY, vertexPosZ + cellPosZ);

                f32 u = static_cast<f32>(vertex % 17);
                f32 v = Math::Floor(static_cast<f32>(vertex) / 17.0f);

                // Handle UV offsets for the inner array
                if (u > 8.01f) 
                {
                    v = v + 0.5f;
                    u = u - 8.5f;
                }

                chunkVertices[vertex + cellOffset].texCoord = vec2(u, v);
                chunkVertices[vertex + cellOffset].normal = vec3(0.0f, 1.0f, 0.0f); // TODO: Actual normals for  terrain

                vertex++;
            }
        }
    }

    // Indices
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
            chunkIndices.push_back(centerVertex);
            chunkIndices.push_back(topLeftVertex);
            chunkIndices.push_back(topRightVertex);

            // Left triangle
            chunkIndices.push_back(centerVertex);
            chunkIndices.push_back(bottomLeftVertex);
            chunkIndices.push_back(topLeftVertex);

            // Down triangle
            chunkIndices.push_back(centerVertex);
            chunkIndices.push_back(bottomRightVertex);
            chunkIndices.push_back(bottomLeftVertex);

            // Right triangle
            chunkIndices.push_back(centerVertex);
            chunkIndices.push_back(topRightVertex);
            chunkIndices.push_back(bottomRightVertex);
        }
    }

    Renderer::ModelID chunkModel = _renderer->CreatePrimitiveModel(chunkModelDesc);
    _chunkModels.push_back(chunkModel);

    // Load all the textures, and combine them into a texture array
    Renderer::InstanceData chunkInstance;
    chunkInstance.Init(_renderer);
    
    // Move the chunk to its proper position
    f32 x = static_cast<f32>(chunkPosX)* Terrain::MAP_CHUNK_SIZE;
    f32 z = static_cast<f32>(chunkPosY)* Terrain::MAP_CHUNK_SIZE; // chunkPosY is intended and not a typo, we're converting from 2d chunk positions to 3d where Y is height

    vec3 chunkPosition = vec3(x, 0.0f, z);
    //const mat4x4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
    const mat4x4 translationMatrix = glm::translate(glm::mat4(1.0f), chunkPosition);
    chunkInstance.modelMatrix = translationMatrix;// *rotationMatrix;

    _chunkModelInstances.push_back(chunkInstance);
}

void TerrainRenderer::LoadChunksAround(Terrain::Map& map, ivec2 middleChunk, u16 radius)
{
    ivec2 startPos = ivec2(middleChunk.x - radius, middleChunk.y - radius);
    ivec2 endPos = ivec2(middleChunk.x + radius, middleChunk.y + radius);

    for(i32 x = startPos.x; x < endPos.x; x++)
    {
        for (i32 y = startPos.y; y < endPos.y; y++)
        {
            LoadChunk(map, x, y);
        }
    }

}
