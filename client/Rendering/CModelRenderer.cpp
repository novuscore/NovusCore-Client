#include "CModelRenderer.h"
#include "DebugRenderer.h"
#include "../Utils/ServiceLocator.h"
#include "../Rendering/CModel/CModel.h"

#include <filesystem>
#include <GLFW/glfw3.h>

#include <InputManager.h>
#include <Renderer/Renderer.h>
#include <Utils/FileReader.h>
#include <Utils/ByteBuffer.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "../ECS/Components/Singletons/DBCSingleton.h"
#include "../ECS/Components/Singletons/TextureSingleton.h"
#include "../ECS/Components/Singletons/DisplayInfoSingleton.h"

#include "../Loaders/NDBC/NDBC.h"

#include <tracy/TracyVulkan.hpp>
#include "../Gameplay/Map/Map.h"

namespace fs = std::filesystem;

CModelRenderer::CModelRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer)
    : _renderer(renderer)
    , _debugRenderer(debugRenderer)
{
    CreatePermanentResources();
}

CModelRenderer::~CModelRenderer()
{

}

void CModelRenderer::Update(f32 deltaTime)
{
}

void CModelRenderer::AddComplexModelPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
    struct CModelPassData
    {
        Renderer::RenderPassMutableResource mainColor;
        Renderer::RenderPassMutableResource mainDepth;
    };

    const auto setup = [=](CModelPassData& data, Renderer::RenderGraphBuilder& builder)
    {
        data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
        data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);

        return true; // Return true from setup to enable this pass, return false to disable it
    };

    const auto execute = [=](CModelPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList)
    {
        GPU_SCOPED_PROFILER_ZONE(commandList, NM2Pass);

        Renderer::GraphicsPipelineDesc pipelineDesc;
        resources.InitializePipelineDesc(pipelineDesc);

        // Shaders
        Renderer::VertexShaderDesc vertexShaderDesc;
        vertexShaderDesc.path = "Data/shaders/cModel.vs.hlsl.spv";
        pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

        Renderer::PixelShaderDesc pixelShaderDesc;
        pixelShaderDesc.path = "Data/shaders/cModel.ps.hlsl.spv";
        pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

        // Depth state
        pipelineDesc.states.depthStencilState.depthEnable = true;
        pipelineDesc.states.depthStencilState.depthWriteEnable = true;
        pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_GREATER;

        // Rasterizer state
        pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK; //Renderer::CullMode::CULL_MODE_BACK;
        pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

        pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
        pipelineDesc.states.blendState.renderTargets[0].blendOp = Renderer::BlendOp::BLEND_OP_ADD;
        pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::BLEND_MODE_SRC_ALPHA;
        pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::BLEND_MODE_INV_SRC_ALPHA;

        // Render targets
        pipelineDesc.renderTargets[0] = data.mainColor;
        pipelineDesc.depthStencil = data.mainDepth;

        // Set Backface Culled Pipeline
        { 
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

            _passDescriptorSet.Bind("_drawCallDatas", _drawCallDataBuffer);
            _passDescriptorSet.Bind("_vertices", _vertexBuffer);
            _passDescriptorSet.Bind("_textures", _cModelTextures);
            _passDescriptorSet.Bind("_textureUnits", _textureUnitBuffer);
            _passDescriptorSet.Bind("_instances", _instanceBuffer);
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);

            u32 numDrawCalls = static_cast<u32>(_drawCalls.size());
            commandList.DrawIndexedIndirect(_drawCallBuffer, 0, numDrawCalls);

            commandList.EndPipeline(pipeline);
        }
       
        // Set No Backface Culled Pipeline
        {
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_NONE;

            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

            _passDescriptorSet.Bind("_drawCallDatas", _twoSidedDrawCallDataBuffer);
            _passDescriptorSet.Bind("_vertices", _vertexBuffer);
            _passDescriptorSet.Bind("_textures", _cModelTextures);
            _passDescriptorSet.Bind("_textureUnits", _textureUnitBuffer);
            _passDescriptorSet.Bind("_instances", _instanceBuffer);
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);

            u32 numDrawCalls = static_cast<u32>(_twoSidedDrawCalls.size());
            commandList.DrawIndexedIndirect(_twoSidedDrawCallBuffer, 0, numDrawCalls);

            commandList.EndPipeline(pipeline);
        }
    };

    renderGraph->AddPass<CModelPassData>("CModel Pass", setup, execute);
}

void CModelRenderer::RegisterLoadFromChunk(const Terrain::Chunk& chunk, StringTable& stringTable)
{
    for (const Terrain::Placement& placement : chunk.complexModelPlacements)
    {
        ComplexModelToBeLoaded& modelToBeLoaded = _complexModelsToBeLoaded.emplace_back();
        modelToBeLoaded.placement = &placement;
        modelToBeLoaded.name = &stringTable.GetString(placement.nameID);
        modelToBeLoaded.nameHash = stringTable.GetStringHash(placement.nameID);
    }
}

void CModelRenderer::ExecuteLoad()
{
    for (ComplexModelToBeLoaded& modelToBeLoaded : _complexModelsToBeLoaded)
    {
        // Placements reference a path to a ComplexModel, several placements can reference the same object
        // Because of this we want only the first load to actually load the object, subsequent loads should reuse the loaded version
        u32 modelID;

        auto it = _nameHashToIndexMap.find(modelToBeLoaded.nameHash);
        if (it == _nameHashToIndexMap.end())
        {
            modelID = static_cast<u32>(_loadedComplexModels.size());
            LoadedComplexModel& complexModel = _loadedComplexModels.emplace_back();
            LoadComplexModel(modelToBeLoaded, complexModel);

            _nameHashToIndexMap[modelToBeLoaded.nameHash] = modelID;
        }
        else
        {
            modelID = it->second;
        }

        // Add placement as an instance
        AddInstance(_loadedComplexModels[modelID], *modelToBeLoaded.placement);
    }

    CreateBuffers();
    _complexModelsToBeLoaded.clear();
}

void CModelRenderer::Clear()
{
    _loadedComplexModels.clear();
    _nameHashToIndexMap.clear();

    _vertices.clear();
    _indices.clear();
    _textureUnits.clear();
    _instances.clear();

    _drawCalls.clear();
    _drawCallDatas.clear();

    _twoSidedDrawCalls.clear();
    _twoSidedDrawCallDatas.clear();

    _renderer->UnloadTexturesInArray(_cModelTextures, 0);
}

void CModelRenderer::CreatePermanentResources()
{
    Renderer::TextureArrayDesc textureArrayDesc;
    textureArrayDesc.size = 4096;

    _cModelTextures = _renderer->CreateTextureArray(textureArrayDesc);

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

    _meshDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());

    /*u32 objectID = 0;
    if (LoadCModel("Creature/ArthasLichKing/ArthasLichKing.cmodel", objectID))
    {
        LoadedCModel& cModel = _loadedCModels[objectID];
        for (i32 y = 0; y < 1; y++)
        {
            for (i32 x = 0; x < 1; x++)
            {
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

                //vec3 pos = vec3(-7900 + (y * 1.5f), 100.f, 1600.f + (x * 2));
                vec3 pos = vec3(-8000.f + (y * 1.5f), 100.f, 1600.f + (x * 2));
                mat4x4 rotationMatrix = glm::eulerAngleXYZ(glm::radians(0.f), glm::radians(45.f), glm::radians(0.f));

                dst->instanceMatrix = glm::translate(mat4x4(1.0f), pos) * rotationMatrix;
                _renderer->UnmapBuffer(stagingBuffer);

                u32 dstOffset = sizeof(Instance) * cModel.numInstances++;

                // Queue destroy staging buffer
                _renderer->QueueDestroyBuffer(stagingBuffer);

                // Copy from staging buffer to buffer
                _renderer->CopyBuffer(cModel.instanceBuffer, dstOffset, stagingBuffer, 0, bufferSize);
            }
        }
    }*/
}

bool CModelRenderer::LoadComplexModel(ComplexModelToBeLoaded& toBeLoaded, LoadedComplexModel& complexModel)
{
    const std::string& modelPath = *toBeLoaded.name;
    
    CModel::ComplexModel cModel;
    if (!LoadFile(modelPath, cModel))
        return false;

    entt::registry* registry = ServiceLocator::GetGameRegistry();
    TextureSingleton& textureSingleton = registry->ctx<TextureSingleton>();

    // Add vertices
    size_t numVerticesBeforeAdd = _vertices.size();
    size_t numVerticesToAdd = cModel.vertices.size();

    _vertices.resize(numVerticesBeforeAdd + numVerticesToAdd);
    memcpy(&_vertices[numVerticesBeforeAdd], cModel.vertices.data(), numVerticesToAdd * sizeof(CModel::ComplexVertex));

    // Handle this models renderbatches
    size_t numRenderBatches = static_cast<u32>(cModel.modelData.renderBatches.size());
    for (size_t i = 0; i < numRenderBatches; i++)
    {
        CModel::ComplexRenderBatch& renderBatch = cModel.modelData.renderBatches[i];

        // Select where to store the DrawCall templates, this won't be necessary once we do backface culling in the culling compute shader
        bool isTwoSided = IsRenderBatchTwoSided(renderBatch, cModel);
        std::vector<DrawCall>& drawCallTemplates = (isTwoSided) ? complexModel.twoSidedDrawCallTemplates : complexModel.drawCallTemplates;
        std::vector<DrawCallData>& drawCallDataTemplates = (isTwoSided) ? complexModel.twoSidedDrawCallDataTemplates : complexModel.drawCallDataTemplates;

        if (isTwoSided)
        {
            complexModel.numTwoSidedDrawCalls++;
        }
        else
        {
            complexModel.numDrawCalls++;
        }

        // For each renderbatch we want to create a template DrawCall and DrawCallData inside of the LoadedComplexModel
        DrawCall& drawCallTemplate = drawCallTemplates.emplace_back();
        DrawCallData& drawCallDataTemplate = drawCallDataTemplates.emplace_back();
        
        drawCallTemplate.instanceCount = 1;
        drawCallTemplate.vertexOffset = static_cast<u32>(numVerticesBeforeAdd);

        // Add indices
        size_t numIndicesBeforeAdd = _indices.size();
        size_t numIndicesToAdd = renderBatch.indexCount;

        _indices.resize(numIndicesBeforeAdd + numIndicesToAdd);
        memcpy(&_indices[numIndicesBeforeAdd], &cModel.modelData.indices[renderBatch.indexStart], numIndicesToAdd * sizeof(u16));

        drawCallTemplate.firstIndex = static_cast<u32>(numIndicesBeforeAdd);
        drawCallTemplate.indexCount = static_cast<u32>(numIndicesToAdd);

        // Add texture units
        size_t numTextureUnitsBeforeAdd = _textureUnits.size();
        size_t numTextureUnitsToAdd = renderBatch.textureUnits.size();

        _textureUnits.resize(numTextureUnitsBeforeAdd + numTextureUnitsToAdd);
        for (size_t j = 0; j < numTextureUnitsToAdd; j++)
        {
            TextureUnit& textureUnit = _textureUnits[numTextureUnitsBeforeAdd + j];

            CModel::ComplexTextureUnit& complexTextureUnit = renderBatch.textureUnits[j];
            CModel::ComplexMaterial& complexMaterial = cModel.materials[complexTextureUnit.materialIndex];
            
            bool isProjectedTexture = (static_cast<u8>(complexTextureUnit.flags) & static_cast<u8>(CModel::ComplexTextureUnitFlag::PROJECTED_TEXTURE)) != 0;
            u16 materialFlag = *reinterpret_cast<u16*>(&complexMaterial.flags) << 1;
            u16 blendingMode = complexMaterial.blendingMode << 11;

            textureUnit.data = static_cast<u16>(isProjectedTexture) | materialFlag | blendingMode;
            textureUnit.materialType = complexTextureUnit.shaderId;

            // Load Textures into Texture Array
            {
                // TODO: Wotlk only supports 2 textures, when we upgrade to cata+ this might need to be reworked
                for (u32 t = 0; t < complexTextureUnit.textureCount; t++)
                {
                    // Load Texture
                    CModel::ComplexTexture& complexTexture = cModel.textures[complexTextureUnit.textureIndices[t]];

                    Renderer::TextureDesc textureDesc;
                    textureDesc.path = "Data/extracted/Textures/" + textureSingleton.textureStringTable.GetString(complexTexture.textureNameIndex);
                    _renderer->LoadTextureIntoArray(textureDesc, _cModelTextures, textureUnit.textureIds[t]);
                }
            }
        }

        drawCallDataTemplate.textureUnitOffset = static_cast<u32>(numTextureUnitsBeforeAdd);
        drawCallDataTemplate.numTextureUnits = static_cast<u32>(numTextureUnitsToAdd);
    }

    return true;
}

bool CModelRenderer::LoadFile(const std::string& cModelPathString, CModel::ComplexModel& cModel)
{
    if (!StringUtils::EndsWith(cModelPathString, ".cmodel"))
    {
        NC_LOG_FATAL("Tried to call 'LoadCModel' with a reference to a file that didn't end with '.cmodel'");
        return false;
    }

    fs::path cModelPath = "Data/extracted/CModels/" + cModelPathString;
    cModelPath.make_preferred();
    cModelPath = fs::absolute(cModelPath);

    FileReader cModelFile(cModelPath.string(), cModelPath.filename().string());
    if (!cModelFile.Open())
    {
        NC_LOG_FATAL("Failed to open CModel file: %s", cModelPath.string().c_str());
        return false;
    }

    Bytebuffer cModelBuffer(nullptr, cModelFile.Length());
    cModelFile.Read(&cModelBuffer, cModelBuffer.size);
    cModelFile.Close();

    if (!cModelBuffer.Get(cModel.header))
        return false;

    if (!cModelBuffer.Get(cModel.flags))
        return false;

    // Read Vertices
    {
        u32 numVertices = 0;
        if (!cModelBuffer.GetU32(numVertices))
            return false;

        // If there are no vertices, we don't need to render it
        if (numVertices == 0)
            return false;

        cModel.vertices.resize(numVertices);
        cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.vertices.data()), numVertices * sizeof(CModel::ComplexVertex));
    }

    // Read Textures
    {
        u32 numTextures = 0;
        if (!cModelBuffer.GetU32(numTextures))
            return false;

        if (numTextures > 0)
        {
            cModel.textures.resize(numTextures);
            cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.textures.data()), numTextures * sizeof(CModel::ComplexTexture));
        }
    }

    // Read Materials
    {
        u32 numMaterials = 0;
        if (!cModelBuffer.GetU32(numMaterials))
            return false;

        if (numMaterials > 0)
        {
            cModel.materials.resize(numMaterials);
            cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.materials.data()), numMaterials * sizeof(CModel::ComplexMaterial));
        }
    }

    // Read Texture Index Lookup Table
    {
        u32 numElements = 0;
        if (!cModelBuffer.GetU32(numElements))
            return false;

        if (numElements > 0)
        {
            cModel.textureIndexLookupTable.resize(numElements);
            cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.textureIndexLookupTable.data()), numElements * sizeof(u16));
        }
    }

    // Read Texture Unit Lookup Table
    {
        u32 numElements = 0;
        if (!cModelBuffer.GetU32(numElements))
            return false;

        if (numElements > 0)
        {
            cModel.textureUnitLookupTable.resize(numElements);
            cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.textureUnitLookupTable.data()), numElements * sizeof(u16));
        }
    }

    // Read Texture Transparency Lookup Table
    {
        u32 numElements = 0;
        if (!cModelBuffer.GetU32(numElements))
            return false;

        if (numElements > 0)
        {
            cModel.textureTransparencyLookupTable.resize(numElements);
            cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.textureTransparencyLookupTable.data()), numElements * sizeof(u16));
        }
    }

    // Read Texture Combiner Combos
    {
        u32 numElements = 0;
        if (!cModelBuffer.GetU32(numElements))
            return false;

        if (numElements > 0)
        {
            cModel.textureCombinerCombos.resize(numElements);
            cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.textureCombinerCombos.data()), numElements * sizeof(u16));
        }
    }

    // Read Model Data
    {
        if (!cModelBuffer.Get(cModel.modelData.header))
            return false;

        // Read Vertex Lookup Ids
        {
            u32 numElements = 0;
            if (!cModelBuffer.GetU32(numElements))
                return false;

            if (numElements > 0)
            {
                cModel.modelData.vertexLookupIds.resize(numElements);
                cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.modelData.vertexLookupIds.data()), numElements * sizeof(u16));
            }
        }

        // Read Indices
        {
            u32 numElements = 0;
            if (!cModelBuffer.GetU32(numElements))
                return false;

            if (numElements > 0)
            {
                cModel.modelData.indices.resize(numElements);
                cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.modelData.indices.data()), numElements * sizeof(u16));
            }
        }

        // Read Render Batches
        {
            u32 numRenderBatches = 0;
            if (!cModelBuffer.GetU32(numRenderBatches))
                return false;

            cModel.modelData.renderBatches.reserve(numRenderBatches);
            for (u32 i = 0; i < numRenderBatches; i++)
            {
                CModel::ComplexRenderBatch& renderBatch = cModel.modelData.renderBatches.emplace_back();

                if (!cModelBuffer.GetU16(renderBatch.groupId))
                    return false;

                if (!cModelBuffer.GetU32(renderBatch.vertexStart))
                    return false;

                if (!cModelBuffer.GetU32(renderBatch.vertexCount))
                    return false;

                if (!cModelBuffer.GetU32(renderBatch.indexStart))
                    return false;

                if (!cModelBuffer.GetU32(renderBatch.indexCount))
                    return false;

                // Read Texture Units
                {
                    u32 numTextureUnits = 0;
                    if (!cModelBuffer.GetU32(numTextureUnits))
                        return false;

                    renderBatch.textureUnits.reserve(numTextureUnits);

                    for (u32 j = 0; j < numTextureUnits; j++)
                    {
                        CModel::ComplexTextureUnit& textureUnit = renderBatch.textureUnits.emplace_back();

                        if (!cModelBuffer.Get(textureUnit.flags))
                            return false;

                        if (!cModelBuffer.GetU16(textureUnit.shaderId))
                            return false;

                        if (!cModelBuffer.GetU16(textureUnit.materialIndex))
                            return false;

                        if (!cModelBuffer.GetU16(textureUnit.materialLayer))
                            return false;

                        if (!cModelBuffer.GetU16(textureUnit.textureCount))
                            return false;

                        if (!cModelBuffer.GetBytes(reinterpret_cast<u8*>(&textureUnit.textureIndices), textureUnit.textureCount * sizeof(u16)))
                            return false;

                        if (!cModelBuffer.GetBytes(reinterpret_cast<u8*>(&textureUnit.textureUVAnimationIndices), textureUnit.textureCount * sizeof(u16)))
                            return false;

                        if (!cModelBuffer.GetU16(textureUnit.textureUnitLookupId))
                            return false;
                    }
                }
            }
        }
    }

    return true;
}

bool CModelRenderer::IsRenderBatchTwoSided(const CModel::ComplexRenderBatch& renderBatch, const CModel::ComplexModel& cModel)
{
    if (renderBatch.textureUnits.size() > 0)
    {
        const CModel::ComplexMaterial& complexMaterial = cModel.materials[renderBatch.textureUnits[0].materialIndex];
        return complexMaterial.flags.disableBackfaceCulling;
    }

    return false;
}

void CModelRenderer::AddInstance(LoadedComplexModel& complexModel, const Terrain::Placement& placement)
{
    // Add the instance
    size_t numInstancesBeforeAdd = _instances.size();
    Instance& instance = _instances.emplace_back();

    vec3 pos = placement.position;
    pos = vec3(Terrain::MAP_HALF_SIZE - pos.x, pos.y, Terrain::MAP_HALF_SIZE - pos.z); // Go from [0 .. MAP_SIZE] to [-MAP_HALF_SIZE .. MAP_HALF_SIZE]
    pos = vec3(pos.z, pos.y, pos.x); // Swizzle and invert x and z

    vec3 rot = glm::radians(placement.rotation);
    rot = vec3(rot.z, -rot.y, rot.x);

    vec3 scale = vec3(placement.scale) / 1024.0f;

    mat4x4 rotationMatrix = glm::eulerAngleXYZ(rot.x, rot.y, rot.z);
    mat4x4 scaleMatrix = glm::scale(mat4x4(1.0f), scale);

    instance.instanceMatrix = glm::translate(mat4x4(1.0f), pos) * rotationMatrix * scaleMatrix;

    // Add the DrawCalls and DrawCallDatas
    size_t numDrawCallsBeforeAdd = _drawCalls.size();
    for (u32 i = 0; i < complexModel.numDrawCalls; i++)
    {
        const DrawCall& drawCallTemplate = complexModel.drawCallTemplates[i];
        const DrawCallData& drawCallDataTemplate = complexModel.drawCallDataTemplates[i];

        DrawCall& drawCall = _drawCalls.emplace_back();
        DrawCallData& drawCallData = _drawCallDatas.emplace_back();

        // Copy data from the templates
        drawCall.firstIndex = drawCallTemplate.firstIndex;
        drawCall.indexCount = drawCallTemplate.indexCount;
        drawCall.instanceCount = drawCallTemplate.instanceCount;
        drawCall.vertexOffset = drawCallTemplate.vertexOffset;

        drawCallData.textureUnitOffset = drawCallDataTemplate.textureUnitOffset;
        drawCallData.numTextureUnits = drawCallDataTemplate.numTextureUnits;
        
        // Fill in the data that shouldn't be templated
        drawCall.firstInstance = static_cast<u32>(numDrawCallsBeforeAdd + i); // This is used in the shader to retrieve the DrawCallData
        drawCallData.instanceID = static_cast<u32>(numInstancesBeforeAdd);
    }

    // Add the DrawCalls and DrawCallDatas for twosided drawcalls
    size_t numTwoSidedDrawCallsBeforeAdd = _twoSidedDrawCalls.size();
    for (u32 i = 0; i < complexModel.numTwoSidedDrawCalls; i++)
    {
        const DrawCall& drawCallTemplate = complexModel.twoSidedDrawCallTemplates[i];
        const DrawCallData& drawCallDataTemplate = complexModel.twoSidedDrawCallDataTemplates[i];

        DrawCall& drawCall = _twoSidedDrawCalls.emplace_back();
        DrawCallData& drawCallData = _twoSidedDrawCallDatas.emplace_back();

        // Copy data from the templates
        drawCall.firstIndex = drawCallTemplate.firstIndex;
        drawCall.indexCount = drawCallTemplate.indexCount;
        drawCall.instanceCount = drawCallTemplate.instanceCount;
        drawCall.vertexOffset = drawCallTemplate.vertexOffset;

        drawCallData.textureUnitOffset = drawCallDataTemplate.textureUnitOffset;
        drawCallData.numTextureUnits = drawCallDataTemplate.numTextureUnits;

        // Fill in the data that shouldn't be templated
        drawCall.firstInstance = static_cast<u32>(numTwoSidedDrawCallsBeforeAdd + i); // This is used in the shader to retrieve the DrawCallData
        drawCallData.instanceID = static_cast<u32>(numInstancesBeforeAdd);
    }
}

void CModelRenderer::CreateBuffers()
{
    // Create Vertex buffer
    if (_vertexBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_vertexBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelVertexBuffer";
        desc.size = sizeof(CModel::ComplexVertex) * _vertices.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _vertexBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "CModelVertexStaging";
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
    }

    // Create Index buffer
    if (_indexBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_indexBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelIndexBuffer";
        desc.size = sizeof(u16) * _indices.size();
        desc.usage = Renderer::BUFFER_USAGE_INDEX_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _indexBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "CModelIndexStaging";
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

    // Create TextureUnit buffer
    if (_textureUnitBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_textureUnitBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelTextureUnitBuffer";
        desc.size = sizeof(TextureUnit) * _textureUnits.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _textureUnitBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "CModelTextureUnitStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _textureUnits.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_textureUnitBuffer, 0, stagingBuffer, 0, desc.size);
    }

    // Create Instance buffer
    if (_instanceBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_instanceBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelInstanceBuffer";
        desc.size = sizeof(Instance) * _instances.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _instanceBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "CModelInstanceStaging";
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
    }

    // Create DrawCall buffer
    if (_drawCallBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_drawCallBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelDrawCallBuffer";
        desc.size = sizeof(DrawCall) * _drawCalls.size();
        desc.usage = Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _drawCallBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "CModelDrawCallStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _drawCalls.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_drawCallBuffer, 0, stagingBuffer, 0, desc.size);
    }

    // Create DrawCall buffer for twosided drawcalls
    if (_twoSidedDrawCallBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_twoSidedDrawCallBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "CModel2SidedDrawCallBuffer";
        desc.size = sizeof(DrawCall) * _twoSidedDrawCalls.size();
        desc.usage = Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _twoSidedDrawCallBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "CModel2SidedDrawCallStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _twoSidedDrawCalls.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_twoSidedDrawCallBuffer, 0, stagingBuffer, 0, desc.size);
    }

    // Create DrawCallData buffer
    if (_drawCallDataBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_drawCallDataBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelDrawCallDataBuffer";
        desc.size = sizeof(DrawCallData) * _drawCallDatas.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _drawCallDataBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "CModelDrawCallDataStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _drawCallDatas.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_drawCallDataBuffer, 0, stagingBuffer, 0, desc.size);
    }

    // Create DrawCallData buffer for twosided drawcalls
    if (_twoSidedDrawCallDataBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_twoSidedDrawCallDataBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelDrawCallDataBuffer";
        desc.size = sizeof(DrawCallData) * _twoSidedDrawCallDatas.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _twoSidedDrawCallDataBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "CModel2SidedDrawCallDataStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _twoSidedDrawCallDatas.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_twoSidedDrawCallDataBuffer, 0, stagingBuffer, 0, desc.size);
    }
}
