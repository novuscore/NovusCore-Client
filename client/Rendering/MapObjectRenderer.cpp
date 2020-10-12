#include "MapObjectRenderer.h"
#include "DebugRenderer.h"
#include <filesystem>
#include <Renderer/Renderer.h>
#include <Utils/FileReader.h>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "../ECS/Components/Singletons/TextureSingleton.h"

#include "Camera.h"
#include "../Gameplay/Map/Map.h"
#include "../Gameplay/Map/Chunk.h"
#include "../Gameplay/Map/MapObjectRoot.h"
#include "../Gameplay/Map/MapObject.h"
#include "../Utils/ServiceLocator.h"
#include "CVar/CVarSystem.h"

namespace fs = std::filesystem;

AutoCVar_Int CVAR_MapObjectCullingEnabled("mapObjects.cullEnable", "enable culling of map objects", 1, CVarFlags::EditCheckbox);
AutoCVar_Int CVAR_MapObjectLockCullingFrustum("mapObjects.lockCullingFrustum", "lock frustrum for map object culling", 0, CVarFlags::EditCheckbox);
AutoCVar_Int CVAR_MapObjectDrawBoundingBoxes("mapObjects.drawBoundingBoxes", "draw bounding boxes for mapobjects", 0, CVarFlags::EditCheckbox);

MapObjectRenderer::MapObjectRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer)
    : _renderer(renderer)
    , _debugRenderer(debugRenderer)
{
    CreatePermanentResources();
}

void GetFrustumPlanes(const mat4x4& m, vec4* planes)
{
    planes[0] = (m[3] + m[0]);
    planes[1] = (m[3] - m[0]);
    planes[2] = (m[3] + m[1]);
    planes[3] = (m[3] - m[1]);
    planes[4] = (m[3] + m[2]);
    planes[5] = (m[3] - m[2]);
}

void MapObjectRenderer::Update(f32 deltaTime)
{
    bool drawBoundingBoxes = CVAR_MapObjectDrawBoundingBoxes.Get() == 1;
    if (drawBoundingBoxes)
    {
        // Draw bounding boxes
        for (u32 i = 0; i < _drawParameters.size(); i++)
        {
            DrawParameters& drawParameters = _drawParameters[i];
            u32 instanceID = drawParameters.firstInstance;

            InstanceLookupData& instanceLookupData = _instanceLookupData[instanceID];

            InstanceData& instanceData = _instances[instanceLookupData.instanceID];

            Terrain::CullingData& cullingData = _cullingData[instanceLookupData.cullingDataID];

            vec3 center = (cullingData.minBoundingBox + cullingData.maxBoundingBox) * f16(0.5f);
            vec3 extents = vec3(cullingData.maxBoundingBox) - center;

            // transform center
            mat4x4& m = instanceData.instanceMatrix;
            vec3 transformedCenter = vec3(m * vec4(center, 1.0f));

            // Transform extents (take maximum)
            glm::mat3x3 absMatrix = glm::mat3x3(glm::abs(vec3(m[0])), glm::abs(vec3(m[1])), glm::abs(vec3(m[2])));
            vec3 transformedExtents = absMatrix * extents;

            // Transform to min/max box representation
            vec3 transformedMin = transformedCenter - transformedExtents;
            vec3 transformedMax = transformedCenter + transformedExtents;

            _debugRenderer->DrawAABB3D(transformedMin, transformedMax, 0xff00ffff);
        }
    }
}

void MapObjectRenderer::AddMapObjectPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
    // Map Object Pass
    {
        struct MapObjectPassData
        {
            Renderer::RenderPassMutableResource mainColor;
            Renderer::RenderPassMutableResource mainDepth;
        };

        const bool cullingEnabled = CVAR_MapObjectCullingEnabled.Get();
        const bool lockFrustum = CVAR_MapObjectLockCullingFrustum.Get();

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

            u32 drawCount = static_cast<u32>(_drawParameters.size());

            // -- Cull MapObjects --
            if (cullingEnabled)
            {
                // Reset the counter
                commandList.FillBuffer(_drawCountBuffer, 0, 4, 0);
                commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, _drawCountBuffer);

                // Do culling
                Renderer::ComputePipelineDesc pipelineDesc;
                resources.InitializePipelineDesc(pipelineDesc);

                Renderer::ComputeShaderDesc shaderDesc;
                shaderDesc.path = "Data/shaders/mapObjectCulling.cs.hlsl.spv";
                pipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);

                Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(pipelineDesc);
                commandList.BeginPipeline(pipeline);

                const u32 drawCount = static_cast<u32>(_drawParameters.size());
                if (!lockFrustum)
                {
                    Camera* camera = ServiceLocator::GetCamera();
                    memcpy(_cullingConstantBuffer->resource.frustumPlanes, camera->GetFrustumPlanes(), sizeof(vec4[6]));
                    _cullingConstantBuffer->resource.cameraPos = camera->GetPosition();
                    _cullingConstantBuffer->resource.maxDrawCount = drawCount;
                    _cullingConstantBuffer->Apply(frameIndex);
                }

                _cullingDescriptorSet.Bind("_constants", _cullingConstantBuffer->GetBuffer(frameIndex));
                _cullingDescriptorSet.Bind("_argumentBuffer", _argumentBuffer);
                _cullingDescriptorSet.Bind("_culledArgumentBuffer", _culledArgumentBuffer);
                _cullingDescriptorSet.Bind("_drawCountBuffer", _drawCountBuffer);
                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_cullingDescriptorSet, frameIndex);
                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

                commandList.Dispatch((drawCount + 31) / 32, 1, 1);

                commandList.EndPipeline(pipeline);

                commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _culledArgumentBuffer);
                commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _drawCountBuffer);
            }
            else
            {
                // Reset the counter
                commandList.FillBuffer(_drawCountBuffer, 0, 4, drawCount);
                commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToIndirectArguments, _drawCountBuffer);
            }
            
            // -- Render MapObjects --
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
            //pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
            //pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BLEND_MODE_DEST_ALPHA;
            //pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BLEND_MODE_INV_DEST_ALPHA;
            //pipelineDesc.states.blendState.renderTargets[0].blendOp = Renderer::BLEND_OP_ADD;

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

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);

            Renderer::BufferID argumentBuffer = (cullingEnabled) ? _culledArgumentBuffer : _argumentBuffer;
            commandList.DrawIndexedIndirectCount(argumentBuffer, 0, _drawCountBuffer, 0, drawCount);

            commandList.EndPipeline(pipeline);
        });
    }
}

void MapObjectRenderer::RegisterMapObjectsToBeLoaded(const Terrain::Chunk& chunk, StringTable& stringTable)
{
    for (const Terrain::MapObjectPlacement& mapObjectPlacement : chunk.mapObjectPlacements)
    {
        MapObjectToBeLoaded& mapObjectToBeLoaded = _mapObjectsToBeLoaded.emplace_back();
        mapObjectToBeLoaded.placement = &mapObjectPlacement;
        mapObjectToBeLoaded.nmorName = &stringTable.GetString(mapObjectPlacement.nameID);
        mapObjectToBeLoaded.nmorNameHash = stringTable.GetStringHash(mapObjectPlacement.nameID);
    }
}

void MapObjectRenderer::ExecuteLoad()
{
    size_t numMapObjectsToLoad = _mapObjectsToBeLoaded.size();

    for (MapObjectToBeLoaded& mapObjectToBeLoaded : _mapObjectsToBeLoaded)
    {
        // Placements reference a path to a MapObject, several placements can reference the same object
        // Because of this we want only the first load to actually load the object, subsequent loads should just return the id to the already loaded version
        u32 mapObjectID;

        auto it = _nameHashToIndexMap.find(mapObjectToBeLoaded.nmorNameHash);
        if (it == _nameHashToIndexMap.end())
        {
            mapObjectID = static_cast<u32>(_loadedMapObjects.size());
            LoadedMapObject& mapObject = _loadedMapObjects.emplace_back();
            LoadMapObject(mapObjectToBeLoaded, mapObject);

            _nameHashToIndexMap[mapObjectToBeLoaded.nmorNameHash] = mapObjectID;
        }
        else
        {
            mapObjectID = it->second;
        }
        
        // Add placement as an instance here
        AddInstance(_loadedMapObjects[mapObjectID], mapObjectToBeLoaded.placement);
    }

    CreateBuffers();
    _mapObjectsToBeLoaded.clear();
}

void MapObjectRenderer::Clear()
{
    _loadedMapObjects.clear();
    _nameHashToIndexMap.clear();
    _indices.clear();
    _vertices.clear();
    _drawParameters.clear();
    _instances.clear();
    _instanceLookupData.clear();
    _instanceLookupIDs.clear();
    _materials.clear();
    _materialParameters.clear();
    _cullingData.clear();

    // Unload everything but the first texture in our array
    _renderer->UnloadTexturesInArray(_mapObjectTextures, 1);
}

void MapObjectRenderer::CreatePermanentResources()
{
    Renderer::TextureArrayDesc textureArrayDesc;
    textureArrayDesc.size = 4096;

    _mapObjectTextures = _renderer->CreateTextureArray(textureArrayDesc);
    _passDescriptorSet.Bind("_textures", _mapObjectTextures);

    // Create a 1x1 pixel black texture
    Renderer::DataTextureDesc dataTextureDesc;
    dataTextureDesc.width = 1;
    dataTextureDesc.height = 1;
    dataTextureDesc.format = Renderer::ImageFormat::IMAGE_FORMAT_B8G8R8A8_UNORM;
    dataTextureDesc.data = new u8[4]{ 0, 0, 0, 0 };

    u32 textureID;
    _renderer->CreateDataTextureIntoArray(dataTextureDesc, _mapObjectTextures, textureID);

    delete[] dataTextureDesc.data;

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

    _cullingDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _cullingConstantBuffer = new Renderer::Buffer<CullingConstants>(_renderer, "CullingConstantBuffer", Renderer::BUFFER_USAGE_UNIFORM_BUFFER, Renderer::BufferCPUAccess::WriteOnly);
}

bool MapObjectRenderer::LoadMapObject(MapObjectToBeLoaded& mapObjectToBeLoaded, LoadedMapObject& mapObject)
{
    // Load root
    if (!StringUtils::EndsWith(*mapObjectToBeLoaded.nmorName, ".nmor"))
    {
        NC_LOG_FATAL("For some reason, a Chunk had a MapObjectPlacement with a reference to a file that didn't end with .nmor");
    }

    fs::path nmorPath = "Data/extracted/MapObjects/" + *mapObjectToBeLoaded.nmorName;
    nmorPath.make_preferred();
    nmorPath = fs::absolute(nmorPath);

    LoadRoot(nmorPath, mapObjectToBeLoaded.meshRoot, mapObject);

    // Load meshes
    std::string nmorNameWithoutExtension = mapObjectToBeLoaded.nmorName->substr(0, mapObjectToBeLoaded.nmorName->length() - 5); // Remove .nmor
    std::stringstream ss;

    mapObject.baseVertexOffset = static_cast<u32>(_vertices.size());
    mapObject.baseCullingDataOffset = static_cast<u32>(_cullingData.size());

    for (u32 i = 0; i < mapObjectToBeLoaded.meshRoot.numMeshes; i++)
    {
        ss.clear();
        ss.str("");

        // Load MapObject
        ss << nmorNameWithoutExtension << "_" << std::setw(3) << std::setfill('0') << i << ".nmo";

        fs::path nmoPath = "Data/extracted/MapObjects/" + ss.str();
        nmoPath.make_preferred();
        nmoPath = fs::absolute(nmoPath);

        Mesh& mesh = mapObjectToBeLoaded.meshes.emplace_back();
        LoadMesh(nmoPath, mesh, mapObject);
    }

    static u32 vertexColorTextureCount = 0;

    // Create vertex color texture
    for (u32 i = 0; i < 2; i++)
    {
        u32 vertexColorCount = static_cast<u32>(mapObject.vertexColors[i].size());
        if (vertexColorCount > 0)
        {
            Renderer::DataTextureDesc vertexColorTextureDesc;
            vertexColorTextureDesc.debugName = "VertexColorTexture";
            vertexColorTextureDesc.width = vertexColorCount;
            vertexColorTextureDesc.height = 1;
            vertexColorTextureDesc.format = Renderer::ImageFormat::IMAGE_FORMAT_B8G8R8A8_UNORM;
            vertexColorTextureDesc.data = reinterpret_cast<u8*>(mapObject.vertexColors[i].data());

            _renderer->CreateDataTextureIntoArray(vertexColorTextureDesc, _mapObjectTextures, mapObject.vertexColorTextureIDs[i]);
            vertexColorTextureCount++;
        }
    }

    // Create per-MapObject culling data
    Terrain::CullingData& mapObjectCullingData = _cullingData.emplace_back();

    for (Terrain::CullingData& cullingData : mapObject.cullingData)
    {
        for (u32 i = 0; i < 3; i++)
        {
            if (cullingData.minBoundingBox[i] < mapObjectCullingData.minBoundingBox[i])
            {
                mapObjectCullingData.minBoundingBox[i] = cullingData.minBoundingBox[i];
            }
            if (cullingData.maxBoundingBox[i] > mapObjectCullingData.maxBoundingBox[i])
            {
                mapObjectCullingData.maxBoundingBox[i] = cullingData.maxBoundingBox[i];
            }
        }
    }

    vec3 minPos = mapObjectCullingData.minBoundingBox;
    vec3 maxPos = mapObjectCullingData.maxBoundingBox;

    mapObjectCullingData.boundingSphereRadius = glm::distance(minPos, maxPos) / 2.0f;

    return true;
}

void MapObjectRenderer::LoadRoot(const std::filesystem::path nmorPath, MeshRoot& meshRoot, LoadedMapObject& mapObject)
{
    FileReader nmorFile(nmorPath.string(), nmorPath.filename().string());
    if (!nmorFile.Open())
    {
        NC_LOG_FATAL("Failed to load Map Object Root file: %s", nmorPath.string());
    }

    Bytebuffer buffer(nullptr, nmorFile.Length());
    nmorFile.Read(&buffer, buffer.size);
    nmorFile.Close();

    Terrain::MapObjectRootHeader header;

    // Read header
    buffer.Get<Terrain::MapObjectRootHeader>(header);

    if (header.token != Terrain::MAP_OBJECT_ROOT_TOKEN)
    {
        NC_LOG_FATAL("We opened MapObjectRoot file (%s) with invalid token %u instead of expected token %u", nmorPath.string(), header.token, Terrain::MAP_OBJECT_ROOT_TOKEN);
    }

    if (header.version != Terrain::MAP_OBJECT_ROOT_VERSION)
    {
        NC_LOG_FATAL("We opened MapObjectRoot file (%s) with invalid version %u instead of expected version %u, rerun dataextractor", nmorPath.string(), header.version, Terrain::MAP_OBJECT_ROOT_VERSION);
    }

    // Read number of materials
    buffer.Get<u32>(meshRoot.numMaterials);

    // Read materials
    entt::registry* registry = ServiceLocator::GetGameRegistry();
    TextureSingleton& textureSingleton = registry->ctx<TextureSingleton>();
    mapObject.baseMaterialOffset = static_cast<u32>(_materials.size());

    for (u32 i = 0; i < meshRoot.numMaterials; i++)
    {
        Terrain::MapObjectMaterial mapObjectMaterial;
        buffer.GetBytes(reinterpret_cast<u8*>(&mapObjectMaterial), sizeof(Terrain::MapObjectMaterial));

        Material& material = _materials.emplace_back();
        material.materialType = mapObjectMaterial.materialType;
        material.unlit = mapObjectMaterial.flags.unlit;

        // TransparencyMode 1 means that it checks the alpha of the texture if it should discard the pixel or not
        if (mapObjectMaterial.transparencyMode == 1)
        {
            material.alphaTestVal = 128.0f / 255.0f;
        }

        constexpr u32 maxTexturesPerMaterial = 3;
        for (u32 j = 0; j < maxTexturesPerMaterial; j++)
        {
            if (mapObjectMaterial.textureNameID[j] < std::numeric_limits<u32>().max())
            {
                Renderer::TextureDesc textureDesc;
                textureDesc.path = "Data/extracted/Textures/" + textureSingleton.textureStringTable.GetString(mapObjectMaterial.textureNameID[j]);

                u32 textureID;
                _renderer->LoadTextureIntoArray(textureDesc, _mapObjectTextures, textureID);

                material.textureIDs[j] = static_cast<u16>(textureID);
            }
        }
    }

    // Read number of meshes
    buffer.Get<u32>(meshRoot.numMeshes);
}

void MapObjectRenderer::LoadMesh(const std::filesystem::path nmoPath, Mesh& mesh, LoadedMapObject& mapObject)
{
    FileReader nmoFile(nmoPath.string(), nmoPath.filename().string());
    if (!nmoFile.Open())
    {
        NC_LOG_FATAL("Failed to load Map Object file: %s", nmoPath.string());
    }

    Bytebuffer nmoBuffer(nullptr, nmoFile.Length());
    nmoFile.Read(&nmoBuffer, nmoBuffer.size);
    nmoFile.Close();

    // Read header
    Terrain::MapObjectHeader header;
    nmoBuffer.Get<Terrain::MapObjectHeader>(header);

    if (header.token != Terrain::MAP_OBJECT_TOKEN)
    {
        NC_LOG_FATAL("We opened MapObject file (%s) with invalid token %u instead of expected token %u", nmoPath.string().c_str(), header.token, Terrain::MAP_OBJECT_TOKEN);
    }

    if (header.version != Terrain::MAP_OBJECT_VERSION)
    {
        if (header.version < Terrain::MAP_OBJECT_VERSION)
        {
            NC_LOG_FATAL("We opened MapObject file (%s) with too old version %u instead of expected version %u, rerun dataextractor", nmoPath.string().c_str(), header.version, Terrain::MAP_OBJECT_VERSION);
        }
        else
        {
            NC_LOG_FATAL("We opened MapObject file (%s) with too new version %u instead of expected version %u, update your client", nmoPath.string().c_str(), header.version, Terrain::MAP_OBJECT_VERSION);
        }
    }

    // Read flags
    nmoBuffer.Get<Terrain::MapObjectFlags>(mesh.renderFlags);

    // Read indices and vertices
    LoadIndicesAndVertices(nmoBuffer, mesh, mapObject);

    // Read renderbatches
    LoadRenderBatches(nmoBuffer, mesh, mapObject);
}

void MapObjectRenderer::LoadIndicesAndVertices(Bytebuffer& buffer, Mesh& mesh, LoadedMapObject& mapObject)
{
    mesh.baseIndexOffset = static_cast<u32>(_indices.size());
    mesh.baseVertexOffset = static_cast<u32>(_vertices.size());

    // Read number of indices
    u32 indexCount;
    buffer.Get<u32>(indexCount);

    _indices.resize(mesh.baseIndexOffset + indexCount);

    // Read indices
    buffer.GetBytes(reinterpret_cast<u8*>(_indices.data() + mesh.baseIndexOffset), indexCount * sizeof(u16));
    
    // Read number of vertices
    u32 vertexCount;
    buffer.Get<u32>(vertexCount);
    _vertices.resize(mesh.baseVertexOffset + vertexCount);
    
    // Read vertices
    buffer.GetBytes(reinterpret_cast<u8*>(&_vertices.data()[mesh.baseVertexOffset]), vertexCount * sizeof(Terrain::MapObjectVertex));

    vec3 position = _vertices[0].position;

    // Read number of vertex color sets
    u32 numVertexColorSets;
    buffer.Get<u32>(numVertexColorSets);

    // Vertex colors
    for (u32 i = 0; i < numVertexColorSets; i++)
    {
        // Read number of vertex colors
        u32 numVertexColors;
        buffer.Get<u32>(numVertexColors);

        u32 vertexColorSize = numVertexColors * sizeof(u32);
        
        u32 beforeSize = static_cast<u32>(mapObject.vertexColors[i].size());
        mapObject.vertexColors[i].resize(beforeSize + numVertexColors);

        buffer.GetBytes(reinterpret_cast<u8*>(&mapObject.vertexColors[i][beforeSize]), vertexColorSize);
    }
}

void MapObjectRenderer::LoadRenderBatches(Bytebuffer& buffer, Mesh& mesh, LoadedMapObject& mapObject)
{
    // Read number of triangle data
    u32 numTriangleData;
    buffer.Get<u32>(numTriangleData);

    // Skip triangle data for now
    buffer.SkipRead(numTriangleData * sizeof(Terrain::TriangleData));

    // Read number of RenderBatches
    u32 numRenderBatches;
    buffer.Get<u32>(numRenderBatches);

    u32 renderBatchesSize = static_cast<u32>(mapObject.renderBatches.size());
    mapObject.renderBatches.resize(renderBatchesSize + numRenderBatches);

    // Read RenderBatches
    buffer.GetBytes(reinterpret_cast<u8*>(&mapObject.renderBatches.data()[renderBatchesSize]), numRenderBatches * sizeof(Terrain::RenderBatch));

    mapObject.renderBatchOffsets.reserve(renderBatchesSize + numRenderBatches);
    for (u32 i = 0; i < numRenderBatches; i++)
    {
        RenderBatchOffsets& renderBatchOffsets = mapObject.renderBatchOffsets.emplace_back();
        renderBatchOffsets.baseVertexOffset = mesh.baseVertexOffset;
        renderBatchOffsets.baseIndexOffset = mesh.baseIndexOffset;

        u32 renderBatchIndex = renderBatchesSize + i;
        Terrain::RenderBatch& renderBatch = mapObject.renderBatches[renderBatchIndex];
        // MaterialParameters
        u32 materialParameterID = static_cast<u32>(_materialParameters.size());

        mapObject.materialParameterIDs.push_back(materialParameterID);

        MaterialParameters& materialParameters = _materialParameters.emplace_back();
        materialParameters.materialID = mapObject.baseMaterialOffset + renderBatch.materialID;
        materialParameters.exteriorLit = static_cast<u32>(mesh.renderFlags.exteriorLit || mesh.renderFlags.exterior);
    }

    // Read culling data
    u32 cullingDataSize = static_cast<u32>(mapObject.cullingData.size());

    mapObject.cullingData.resize(cullingDataSize + numRenderBatches);

    buffer.GetBytes(reinterpret_cast<u8*>(&mapObject.cullingData.data()[cullingDataSize]), numRenderBatches * sizeof(Terrain::CullingData));

    static f32 smallestRadius = 100000.0f;
    for (u32 i = cullingDataSize; i < cullingDataSize + numRenderBatches; i++)
    {
        Terrain::CullingData& cullingData = mapObject.cullingData[i];
        if (cullingData.boundingSphereRadius < smallestRadius)
        {
            smallestRadius = cullingData.boundingSphereRadius;
            NC_LOG_MESSAGE("Smallest radius: %f", smallestRadius);
        }
    }
}

void MapObjectRenderer::AddInstance(LoadedMapObject& mapObject, const Terrain::MapObjectPlacement* placement)
{
    u32 instanceID = static_cast<u32>(_instances.size());
    mapObject.instanceIDs.push_back(instanceID);
    
    InstanceData& instance = _instances.emplace_back();

    vec3 pos = placement->position;
    pos = vec3(Terrain::MAP_HALF_SIZE - pos.x, pos.y, Terrain::MAP_HALF_SIZE - pos.z); // Go from [0 .. MAP_SIZE] to [-MAP_HALF_SIZE .. MAP_HALF_SIZE]
    pos = vec3(pos.z, pos.y, pos.x); // Swizzle and invert x and z

    vec3 rot = placement->rotation;
    mat4x4 rotationMatrix = glm::eulerAngleXYZ(glm::radians(rot.z), glm::radians(-rot.y), glm::radians(rot.x));

    instance.instanceMatrix = glm::translate(mat4x4(1.0f), pos) * rotationMatrix;

    for (u32 i = 0; i < mapObject.renderBatches.size(); i++)
    {
        Terrain::RenderBatch& renderBatch = mapObject.renderBatches[i];
        RenderBatchOffsets& renderBatchOffsets = mapObject.renderBatchOffsets[i];

        u32 drawParameterID = static_cast<u32>(_drawParameters.size());
        DrawParameters& drawParameters = _drawParameters.emplace_back();

        mapObject.drawParameterIDs.push_back(drawParameterID);

        drawParameters.vertexOffset = renderBatchOffsets.baseVertexOffset;
        drawParameters.firstIndex = renderBatchOffsets.baseIndexOffset + renderBatch.startIndex;
        drawParameters.indexCount = renderBatch.indexCount;
        drawParameters.firstInstance = drawParameterID;
        drawParameters.instanceCount = 1;

        InstanceLookupData& instanceLookupData = _instanceLookupData.emplace_back();
        instanceLookupData.instanceID = instanceID;
        instanceLookupData.materialParamID = mapObject.materialParameterIDs[i];
        instanceLookupData.cullingDataID = mapObject.baseCullingDataOffset;

        instanceLookupData.vertexColorTextureID0 = static_cast<u16>(mapObject.vertexColorTextureIDs[0]);
        instanceLookupData.vertexColorTextureID1 = static_cast<u16>(mapObject.vertexColorTextureIDs[1]);
        instanceLookupData.vertexOffset = renderBatchOffsets.baseVertexOffset;
    }

    mapObject.instanceCount++;
}

void MapObjectRenderer::CreateBuffers()
{
    // Create Instance Lookup Buffer
    if (_instanceLookupBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_instanceLookupBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "InstanceLookupDataBuffer";
        desc.size = sizeof(InstanceLookupData) * _instanceLookupData.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _instanceLookupBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "InstanceLookupDataStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _instanceLookupData.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_instanceLookupBuffer, 0, stagingBuffer, 0, desc.size);

        _passDescriptorSet.Bind("_instanceLookup", _instanceLookupBuffer);
        _cullingDescriptorSet.Bind("_instanceLookup", _instanceLookupBuffer);
    }
    
    // Create Indirect Argument buffer
    if (_argumentBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_argumentBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "MapObjectIndirectArgs";
        desc.size = sizeof(DrawParameters) * _drawParameters.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION | Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER;
        _argumentBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MapObjectIndirectStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _drawParameters.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to first buffer
        _renderer->CopyBuffer(_argumentBuffer, 0, stagingBuffer, 0, desc.size);
    }

    // Create Culled Indirect Argument buffer
    if (_culledArgumentBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_culledArgumentBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "MapObjectCulledIndirectArgs";
        desc.size = sizeof(DrawParameters) * _drawParameters.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION | Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER;
        _culledArgumentBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MapObjectIndirectStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _drawParameters.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to second buffer
        _renderer->CopyBuffer(_culledArgumentBuffer, 0, stagingBuffer, 0, desc.size);
    }

    // Create draw count buffer
    if (_drawCountBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_drawCountBuffer);
    }
    if (_drawCountBuffer == Renderer::BufferID::Invalid())
    {
        Renderer::BufferDesc desc;
        desc.name = "MapObjectCullingCounter";
        desc.size = sizeof(u32);
        desc.usage = Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER | Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _drawCountBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MapObjectCullingCounterStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        u32 drawCount = static_cast<u32>(_drawParameters.size());
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memset(dst, drawCount, desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_drawCountBuffer, 0, stagingBuffer, 0, desc.size);
    }

    // Create Vertex buffer
    if (_vertexBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_vertexBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "MapObjectVertexBuffer";
        desc.size = sizeof(Terrain::MapObjectVertex) * _vertices.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _vertexBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MapObjectVertexStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _vertices.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_vertexBuffer, 0, stagingBuffer, 0, desc.size);

        _passDescriptorSet.Bind("_vertices", _vertexBuffer);
    }

    // Create Index buffer
    if (_indexBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_indexBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "MapObjectIndexBuffer";
        desc.size = sizeof(u16) * _indices.size();
        desc.usage = Renderer::BUFFER_USAGE_INDEX_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _indexBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MapObjectIndexStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _indices.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_indexBuffer, 0, stagingBuffer, 0, desc.size);
    }

    // Create Instance buffer
    if (_instanceBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_instanceBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "MapObjectInstanceBuffer";
        desc.size = sizeof(InstanceData) * _instances.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _instanceBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MapObjectInstanceStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _instances.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_instanceBuffer, 0, stagingBuffer, 0, desc.size);

        _passDescriptorSet.Bind("_instanceData", _instanceBuffer);
        _cullingDescriptorSet.Bind("_instanceData", _instanceBuffer);
    }

    // Create Material buffer
    if (_materialBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_materialBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "MapObjectMaterialBuffer";
        desc.size = sizeof(Material) * _materials.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _materialBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MapObjectMaterialStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _materials.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_materialBuffer, 0, stagingBuffer, 0, desc.size);

        _passDescriptorSet.Bind("_materialData", _materialBuffer);
    }

    // Create MaterialParam buffer
    if (_materialParametersBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_materialParametersBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "MapObjectMaterialParamBuffer";
        desc.size = sizeof(MaterialParameters) * _materialParameters.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _materialParametersBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MapObjectMaterialParamStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _materialParameters.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_materialParametersBuffer, 0, stagingBuffer, 0, desc.size);

        _passDescriptorSet.Bind("_materialParams", _materialParametersBuffer);
    }

    // Create CullingData buffer
    if (_cullingDataBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_cullingDataBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "MapObjectCullingDataBuffer";
        desc.size = sizeof(Terrain::CullingData) * _cullingData.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _cullingDataBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MapObjectCullingDataStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _cullingData.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_cullingDataBuffer, 0, stagingBuffer, 0, desc.size);

        _cullingDescriptorSet.Bind("_cullingData", _cullingDataBuffer);
    }
}
