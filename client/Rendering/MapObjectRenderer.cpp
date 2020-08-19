#include "MapObjectRenderer.h"
#include <filesystem>
#include <Renderer/Renderer.h>
#include <Utils/FileReader.h>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "../Gameplay/Map/Chunk.h"
#include "../Gameplay/Map/MapObjectRoot.h"
#include "../Gameplay/Map/MapObject.h"


MapObjectRenderer::MapObjectRenderer(Renderer::Renderer* renderer)
    : _renderer(renderer)
{
    CreatePermanentResources();
}

void MapObjectRenderer::Update(f32 deltaTime)
{

}

void MapObjectRenderer::AddMapObjectPass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
    // Map Object Pass
    {
        struct MapObjectPassData
        {
            Renderer::RenderPassMutableResource mainColor;
            Renderer::RenderPassMutableResource mainDepth;
        };

        renderGraph->AddPass<MapObjectPassData>("MapObject Pass",
            [=](MapObjectPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
            data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
            [=](MapObjectPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
        {
            GPU_SCOPED_PROFILER_ZONE(commandList, MapObjectPass);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            resources.InitializePipelineDesc(pipelineDesc);

            // Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/mapObject.vs.hlsl.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "Data/shaders/mapObject.ps.hlsl.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            // Blend state
            /*pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
            pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BLEND_MODE_DEST_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BLEND_MODE_INV_DEST_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].blendOp = Renderer::BLEND_OP_ADD;*/

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

            for (LoadedMapObject& loadedMapObject : _loadedMapObjects)
            {
                commandList.PushMarker("MapObject", Color::White);

                _passDescriptorSet.Bind("ViewData", viewConstantBuffer->GetBuffer(frameIndex));
                _passDescriptorSet.Bind("_instanceData", loadedMapObject.instanceBuffer);
                _passDescriptorSet.Bind("_materialData", loadedMapObject.materialsBuffer);
                _passDescriptorSet.Bind("_textures", _mapObjectTextures);
                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

                for (Mesh& mesh : loadedMapObject.meshes)
                {
                    _meshDescriptorSet.Bind("_vertexPositions", mesh.vertexPositionsBuffer);
                    _meshDescriptorSet.Bind("_vertexNormals", mesh.vertexNormalsBuffer);
                    _meshDescriptorSet.Bind("_vertexUVs", mesh.vertexUVsBuffer);

                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_meshDescriptorSet, frameIndex);

                    for (u32 i = 0; i < mesh.indexBuffers.size(); i++)
                    {
                        commandList.PushConstant(&mesh.materialIDs[i], 0, 12);

                        commandList.SetIndexBuffer(mesh.indexBuffers[i], Renderer::IndexFormat::UInt16);
                        commandList.DrawIndexed(mesh.numIndices[i], loadedMapObject.numInstances, 0, 0, 0);
                    }
                }

                commandList.PopMarker();
            }

            commandList.EndPipeline(pipeline);
        });
    }
}

void MapObjectRenderer::LoadMapObjects(const Terrain::Chunk& chunk, StringTable& stringTable)
{
    for (const Terrain::MapObjectPlacement& mapObjectPlacement : chunk.mapObjectPlacements)
    {
        u32 mapObjectID;
        if (LoadMapObject(mapObjectPlacement.nameID, stringTable, mapObjectID))
        {
            LoadedMapObject& mapObject = _loadedMapObjects[mapObjectID];

            // Create staging buffer
            const u32 bufferSize = sizeof(Instance);
            Renderer::BufferDesc desc;
            desc.name = "InstanceStaging";
            desc.size = bufferSize;
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
            desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

            Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

            // Upload to staging buffer
            Instance* dst = reinterpret_cast<Instance*>(_renderer->MapBuffer(stagingBuffer));

            vec3 pos = mapObjectPlacement.position;
            pos = vec3(Terrain::MAP_HALF_SIZE - pos.x, pos.y, Terrain::MAP_HALF_SIZE - pos.z); // Go from [0 .. MAP_SIZE] to [-MAP_HALF_SIZE .. MAP_HALF_SIZE]
            pos = vec3(pos.z, pos.y, pos.x); // Swizzle and invert x and z

            vec3 rot = mapObjectPlacement.rotation;
            mat4x4 rotationMatrix = glm::eulerAngleXYZ(glm::radians(rot.z), glm::radians(-rot.y), glm::radians(rot.x));

            dst->instanceMatrix = glm::translate(mat4x4(1.0f), pos) * rotationMatrix;
            _renderer->UnmapBuffer(stagingBuffer);

            u32 dstOffset = sizeof(Instance) * mapObject.numInstances++;

            // Queue destroy staging buffer
            _renderer->QueueDestroyBuffer(stagingBuffer);
            // Copy from staging buffer to buffer
            _renderer->CopyBuffer(mapObject.instanceBuffer, dstOffset, stagingBuffer, 0, bufferSize);
        }
    }
}

void MapObjectRenderer::Clear()
{
    _loadedMapObjects.clear();
    _nameHashToIndexMap.clear();
}

void MapObjectRenderer::CreatePermanentResources()
{
    Renderer::TextureArrayDesc textureArrayDesc;
    textureArrayDesc.size = 1024;

    _mapObjectTextures = _renderer->CreateTextureArray(textureArrayDesc);

    Renderer::SamplerDesc samplerDesc;
    samplerDesc.enabled = true;
    samplerDesc.filter = Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_LINEAR;//Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.addressU = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressV = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressW = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_PIXEL;

    _sampler = _renderer->CreateSampler(samplerDesc);

    _passDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _passDescriptorSet.Bind("_sampler", _sampler);
}

namespace fs = std::filesystem;
bool MapObjectRenderer::LoadMapObject(u32 nameID, StringTable& stringTable, u32& objectID)
{
    // Placements reference a path to a MapObject, several placements can reference the same object
    // Because of this we want only the first load to actually load the object, subsequent loads should just return the id to the already loaded version
    u32 nameHash = stringTable.GetStringHash(nameID);

    auto it = _nameHashToIndexMap.find(nameHash);
    if (it != _nameHashToIndexMap.end())
    {
        objectID = it->second;
        return false;
    }

    static u32 objectCount = 0;
    NC_LOG_MESSAGE("Loading object %u", objectCount);
    objectCount++;

    u32 nextID = static_cast<u32>(_loadedMapObjects.size());
    LoadedMapObject& loadedMapObject = _loadedMapObjects.emplace_back();
    loadedMapObject.debugName = stringTable.GetString(nameID);

    // -- Read MapObjectRoot --
    const std::string& nmorName = stringTable.GetString(nameID);
    if (!StringUtils::EndsWith(nmorName, ".nmor"))
    {
        NC_LOG_FATAL("For some reason, a Chunk had a MapObjectPlacement with a reference to a file that didn't end with .nmor");
    }

    fs::path nmorPath = "Data/extracted/MapObjects/" + nmorName;
    nmorPath.make_preferred();
    nmorPath = fs::absolute(nmorPath);

    FileReader nmorFile(nmorPath.string(), nmorPath.filename().string());
    if (!nmorFile.Open())
    {
        NC_LOG_FATAL("Failed to load Map Object Root file: %s", nmorPath.string());
    }

    Bytebuffer nmorBuffer(nullptr, nmorFile.Length());
    nmorFile.Read(&nmorBuffer, nmorBuffer.size);
    nmorFile.Close();

    Terrain::MapObjectRoot mapObjectRoot;

    // Read header
    nmorBuffer.Get<Terrain::MapObjectRootHeader>(mapObjectRoot.header);

    if (mapObjectRoot.header.token != Terrain::MAP_OBJECT_ROOT_TOKEN)
    {
        NC_LOG_FATAL("We opened MapObjectRoot file (%s) with invalid token %u instead of expected token %u", nmorPath.string(), mapObjectRoot.header.token, Terrain::MAP_OBJECT_ROOT_TOKEN);
    }

    if (mapObjectRoot.header.version != Terrain::MAP_OBJECT_ROOT_VERSION)
    {
        NC_LOG_FATAL("We opened MapObjectRoot file (%s) with invalid version %u instead of expected version %u, rerun dataextractor", nmorPath.string(), mapObjectRoot.header.version, Terrain::MAP_OBJECT_ROOT_VERSION);
    }

    // Read number of materials
    u32 numMaterials;
    nmorBuffer.Get<u32>(numMaterials);

    // Read materials
    mapObjectRoot.materials.resize(numMaterials);
    nmorBuffer.GetBytes(reinterpret_cast<u8*>(mapObjectRoot.materials.data()), numMaterials * sizeof(Terrain::MapObjectMaterial));

    // Read number of groups
    nmorBuffer.Get<u32>(mapObjectRoot.numMapObjects);

    StringTable textureStringTable;
    textureStringTable.Deserialize(&nmorBuffer);

    // -- Create Materials Buffer
    {
        constexpr size_t numTexturePerMaterial = 3;

        const size_t oneBufferSize = sizeof(Material);
        const size_t totalBufferSize = numMaterials * oneBufferSize;

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "Materials";
        desc.size = totalBufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        loadedMapObject.materialsBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MaterialsStaging";
        desc.size = totalBufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);

        for (u32 i = 0; i < numMaterials; i++)
        {
            Material renderMaterial;

            Terrain::MapObjectMaterial& material = mapObjectRoot.materials[i];
            renderMaterial.materialType = material.materialType;
            
            // TransparencyMode 1 means that it checks the alpha of the texture to decide if it should discard the pixel or not
            if (material.transparencyMode == 1)
            {
                renderMaterial.alphaTestVal = 128.0f / 255.0f;
            }

            for (u32 j = 0; j < numTexturePerMaterial; j++)
            {
                if (material.textureNameID[j] < Terrain::INVALID_TEXTURE_ID)
                {
                    Renderer::TextureDesc textureDesc;
                    textureDesc.path = "Data/extracted/Textures/" + textureStringTable.GetString(material.textureNameID[j]);

                    _renderer->LoadTextureIntoArray(textureDesc, _mapObjectTextures, renderMaterial.textureIDs[j]);
                }
            }

            size_t offset = i * oneBufferSize;
            memcpy(static_cast<u8*>(dst) + offset, &renderMaterial, oneBufferSize);
        }
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(loadedMapObject.materialsBuffer, 0, stagingBuffer, 0, totalBufferSize);
    }

    // -- Create Instance Buffer
    {
        const size_t bufferSize = MAX_INSTANCES * sizeof(Instance);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "Instance";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        loadedMapObject.instanceBuffer = _renderer->CreateBuffer(desc);
    }

    // -- Read MapObjects --
    std::string nmorNameWithoutExtension = nmorName.substr(0, nmorName.length() - 5); // Remove .nmor
    std::stringstream ss;

    for (u32 i = 0; i < mapObjectRoot.numMapObjects; i++)
    {
        ss.clear();
        ss.str("");

        // Load MapObject
        ss << nmorNameWithoutExtension << "_" << std::setw(3) << std::setfill('0') << i << ".nmo";

        fs::path nmoPath = "Data/extracted/MapObjects/" + ss.str();
        nmoPath.make_preferred();
        nmoPath = fs::absolute(nmoPath);

        FileReader nmoFile(nmoPath.string(), nmoPath.filename().string());
        if (!nmoFile.Open())
        {
            NC_LOG_FATAL("Failed to load Map Object file: %s", nmoPath.string());
        }

        Bytebuffer nmoBuffer(nullptr, nmoFile.Length());
        nmoFile.Read(&nmoBuffer, nmoBuffer.size);
        nmoFile.Close();

        Terrain::MapObject mapObject;

        // Read header
        nmoBuffer.Get<Terrain::MapObjectHeader>(mapObject.header);

        if (mapObject.header.token != Terrain::MAP_OBJECT_TOKEN)
        {
            NC_LOG_FATAL("We opened MapObject file (%s) with invalid token %u instead of expected token %u", nmoPath.string(), mapObject.header.token, Terrain::MAP_OBJECT_TOKEN);
        }

        if (mapObject.header.version != Terrain::MAP_OBJECT_VERSION)
        {
            NC_LOG_FATAL("We opened MapObject file (%s) with invalid version %u instead of expected version %u, rerun dataextractor", nmoPath.string(), mapObject.header.version, Terrain::MAP_OBJECT_VERSION);
        }

        // Read number of indices
        u32 numIndices;
        nmoBuffer.Get<u32>(numIndices);

        mapObject.indices.resize(numIndices);

        // Read indices
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.indices.data()), numIndices * sizeof(u16));

        // Read number of vertices
        u32 numVertices;
        nmoBuffer.Get<u32>(numVertices);

        mapObject.vertexPositions.resize(numVertices);
        mapObject.vertexNormals.resize(numVertices);
        
        // Read vertices
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.vertexPositions.data()), numVertices * sizeof(vec3));
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.vertexNormals.data()), numVertices * sizeof(vec3));

        // Read number of UV sets
        u32 numUVSets;
        nmoBuffer.Get<u32>(numUVSets);

        mapObject.uvSets.reserve(numUVSets);
        for (u32 j = 0; j < numUVSets; j++)
        {
            Terrain::UVSet& uvSet = mapObject.uvSets.emplace_back();
            uvSet.vertexUVs.resize(numVertices);

            nmoBuffer.GetBytes(reinterpret_cast<u8*>(uvSet.vertexUVs.data()), numVertices * sizeof(vec2));
        }

        // Read number of triangle data
        u32 numTriangleData;
        nmoBuffer.Get<u32>(numTriangleData);
        mapObject.triangleData.resize(numTriangleData);

        // Read triangle data
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.triangleData.data()), numTriangleData * sizeof(Terrain::TriangleData));

        // Read number of RenderBatches
        u32 numRenderBatches;
        nmoBuffer.Get<u32>(numRenderBatches);

        if (numRenderBatches == 0)
        {
            continue;
        }

        mapObject.renderBatches.resize(numRenderBatches);

        // Read RenderBatches
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.renderBatches.data()), numRenderBatches * sizeof(Terrain::RenderBatch));

        // Create mesh, each MapObject becomes one Mesh in loadedMapObject
        Mesh& mesh = loadedMapObject.meshes.emplace_back();
        mesh.numUVSets = numUVSets;

        // Find out how many different materials this Mesh uses
        /*std::vector<u32> materialIndexStarts;
        std::vector<u32> materialIndexEnds;
        mesh.materialIDs.reserve(16);
        materialIndexStarts.reserve(16);

        u32 index = 0;
        for (Terrain::TriangleData& triangleData : mapObject.triangleData)
        {
            u8 materialID = triangleData.materialID;

            if (materialID == 255)
            {
                if (materialIndexStarts.size() > 0)
                {
                    materialIndexEnds.push_back(index);
                }
                break;
            }

            bool found = false;

            for (u32 usedMaterialID : mesh.materialIDs)
            {
                if (materialID == usedMaterialID)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                if (materialIndexStarts.size() > 0)
                {
                    materialIndexEnds.push_back(index);
                }

                mesh.materialIDs.push_back(materialID);
                materialIndexStarts.push_back(index);
            }

            index += 3; // Each triangle is 3 vertices
        }

        if (materialIndexStarts.size() > materialIndexEnds.size())
        {
            materialIndexEnds.push_back(index);
        }*/

        std::vector<u32> materialIndexStarts;
        std::vector<u32> materialIndexEnds;

        mesh.materialIDs.reserve(16);
        for (Terrain::RenderBatch& renderBatch : mapObject.renderBatches)
        {
            mesh.materialIDs.push_back(renderBatch.materialID);
            materialIndexStarts.push_back(renderBatch.startIndex);
            u32 indexEnd = renderBatch.startIndex + renderBatch.indexCount;
            materialIndexEnds.push_back(indexEnd);
        }

        u32 numUsedMaterials = static_cast<u32>(materialIndexStarts.size());
        if (numUsedMaterials > 0)
        {
            // -- Create Index Buffers --
            // We want one index buffer per usedMaterialID
            for (u32 i = 0; i < numUsedMaterials; i++)
            {
                Renderer::BufferID& indexBufferID = mesh.indexBuffers.emplace_back();
                u32& numIndices = mesh.numIndices.emplace_back();

                u32 startIndex = materialIndexStarts[i];
                u32 endIndex = materialIndexEnds[i];
                numIndices = endIndex - startIndex;

                const size_t bufferSize = numIndices * sizeof(u16);

                Renderer::BufferDesc desc;
                desc.name = "IndexBuffer";
                desc.size = bufferSize;
                desc.usage = Renderer::BufferUsage::BUFFER_USAGE_INDEX_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
                desc.cpuAccess = Renderer::BufferCPUAccess::None;

                indexBufferID = _renderer->CreateBuffer(desc);

                // Create staging buffer
                desc.name = "IndexBufferStaging";
                desc.size = bufferSize;
                desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
                desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

                Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

                // Upload to staging buffer
                void* src = &mapObject.indices.data()[startIndex];
                void* dst = _renderer->MapBuffer(stagingBuffer);
                memcpy(dst, src, bufferSize);
                _renderer->UnmapBuffer(stagingBuffer);

                // Queue destroy staging buffer
                _renderer->QueueDestroyBuffer(stagingBuffer);
                // Copy from staging buffer to buffer
                _renderer->CopyBuffer(indexBufferID, 0, stagingBuffer, 0, bufferSize);
            }

            // -- Create Vertex Buffers --
            {
                const size_t bufferSize = numVertices * sizeof(vec3);

                // Create buffer
                Renderer::BufferDesc desc;
                desc.name = "VertexPositions";
                desc.size = bufferSize;
                desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
                desc.cpuAccess = Renderer::BufferCPUAccess::None;

                mesh.vertexPositionsBuffer = _renderer->CreateBuffer(desc);

                // Create staging buffer
                desc.name = "VertexPositionsStaging";
                desc.size = bufferSize;
                desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
                desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

                Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

                // Upload to staging buffer
                void* dst = _renderer->MapBuffer(stagingBuffer);
                memcpy(dst, mapObject.vertexPositions.data(), bufferSize);
                _renderer->UnmapBuffer(stagingBuffer);

                // Queue destroy staging buffer
                _renderer->QueueDestroyBuffer(stagingBuffer);
                // Copy from staging buffer to buffer
                _renderer->CopyBuffer(mesh.vertexPositionsBuffer, 0, stagingBuffer, 0, bufferSize);
            }

            {
                const size_t bufferSize = numVertices * sizeof(vec3);

                // Create buffer
                Renderer::BufferDesc desc;
                desc.name = "VertexNormals";
                desc.size = bufferSize;
                desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
                desc.cpuAccess = Renderer::BufferCPUAccess::None;

                mesh.vertexNormalsBuffer = _renderer->CreateBuffer(desc);

                // Create staging buffer
                desc.name = "VertexPositionsStaging";
                desc.size = bufferSize;
                desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
                desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

                Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

                // Upload to staging buffer
                void* dst = _renderer->MapBuffer(stagingBuffer);
                memcpy(dst, mapObject.vertexPositions.data(), bufferSize);
                _renderer->UnmapBuffer(stagingBuffer);

                // Queue destroy staging buffer
                _renderer->QueueDestroyBuffer(stagingBuffer);
                // Copy from staging buffer to buffer
                _renderer->CopyBuffer(mesh.vertexNormalsBuffer, 0, stagingBuffer, 0, bufferSize);
            }

            {
                constexpr size_t maxNumUVsets = 2;
                const size_t bufferSize = numVertices * maxNumUVsets * sizeof(vec2) ;

                // Create buffer
                Renderer::BufferDesc desc;
                desc.name = "VertexUVs";
                desc.size = bufferSize;
                desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
                desc.cpuAccess = Renderer::BufferCPUAccess::None;

                mesh.vertexUVsBuffer = _renderer->CreateBuffer(desc);

                // Create staging buffer
                desc.name = "VertexPositionsStaging";
                desc.size = bufferSize;
                desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
                desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

                Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

                // Upload to staging buffer
                vec2* dst = static_cast<vec2*>(_renderer->MapBuffer(stagingBuffer));

                memset(dst, 0, bufferSize);

                for (u32 uvSet = 0; uvSet < mesh.numUVSets; uvSet++)
                {
                    size_t offset = uvSet;
                    for (u32 vertexID = 0; vertexID < numVertices; vertexID++)
                    {
                        dst[offset] = mapObject.uvSets[uvSet].vertexUVs[vertexID];
                        offset += maxNumUVsets;
                    }
                }

                //memcpy(dst, mapObject.vertexUVs.data(), bufferSize);
                _renderer->UnmapBuffer(stagingBuffer);

                // Queue destroy staging buffer
                _renderer->QueueDestroyBuffer(stagingBuffer);
                // Copy from staging buffer to buffer
                _renderer->CopyBuffer(mesh.vertexUVsBuffer, 0, stagingBuffer, 0, bufferSize);
            }
        }
    }

    objectID = nextID;
    return true;
}
