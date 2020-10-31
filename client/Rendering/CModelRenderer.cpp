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

#include "../Loaders/DBC/DBC.h"

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

void CModelRenderer::AddCModelPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
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

            _passDescriptorSet.Bind("_textures", _cModelTextures);
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            for (LoadedCModel& loadedCModel : _loadedCModels)
            {
                commandList.PushMarker("CModel", Color::White);

                Mesh& mesh = loadedCModel.mesh;

                _meshDescriptorSet.Bind("_instanceData", loadedCModel.instanceBuffer);
                _meshDescriptorSet.Bind("_vertexPositions", mesh.vertexPositionsBuffer);
                _meshDescriptorSet.Bind("_vertexNormals", mesh.vertexNormalsBuffer);
                _meshDescriptorSet.Bind("_vertexUVs0", mesh.vertexUVs0Buffer);
                _meshDescriptorSet.Bind("_vertexUVs1", mesh.vertexUVs1Buffer);
                _meshDescriptorSet.Bind("_textureUnits", mesh.textureUnitsBuffer);
                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_meshDescriptorSet, frameIndex);

                for (size_t i = 0; i < mesh.renderBatches.size(); i++)
                {
                    RenderBatch& renderBatch = mesh.renderBatches[i];

                    if (!renderBatch.isBackfaceCulled)
                        continue;

                    commandList.PushConstant(renderBatch.textureUnitIndices, 0, sizeof(renderBatch.textureUnitIndices));
                    commandList.SetIndexBuffer(renderBatch.indexBuffer, Renderer::IndexFormat::UInt16);
                    commandList.DrawIndexed(renderBatch.indexCount, loadedCModel.numInstances, 0, 0, 0);
                }

                commandList.PopMarker();
            }

            commandList.EndPipeline(pipeline);
        }
       
        // Set No Backface Culled Pipeline
        {
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_NONE;

            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

            _passDescriptorSet.Bind("_textures", _cModelTextures);
            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            for (LoadedCModel& loadedCModel : _loadedCModels)
            {
                commandList.PushMarker("CModel", Color::White);

                Mesh& mesh = loadedCModel.mesh;

                _meshDescriptorSet.Bind("_instanceData", loadedCModel.instanceBuffer);
                _meshDescriptorSet.Bind("_vertexPositions", mesh.vertexPositionsBuffer);
                _meshDescriptorSet.Bind("_vertexNormals", mesh.vertexNormalsBuffer);
                _meshDescriptorSet.Bind("_vertexUVs0", mesh.vertexUVs0Buffer);
                _meshDescriptorSet.Bind("_vertexUVs1", mesh.vertexUVs1Buffer);
                _meshDescriptorSet.Bind("_textureUnits", mesh.textureUnitsBuffer);
                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_meshDescriptorSet, frameIndex);

                for (size_t i = 0; i < mesh.renderBatches.size(); i++)
                {
                    RenderBatch& renderBatch = mesh.renderBatches[i];

                    if (renderBatch.isBackfaceCulled)
                        continue;

                    commandList.PushConstant(renderBatch.textureUnitIndices, 0, sizeof(renderBatch.textureUnitIndices));
                    commandList.SetIndexBuffer(renderBatch.indexBuffer, Renderer::IndexFormat::UInt16);
                    commandList.DrawIndexed(renderBatch.indexCount, loadedCModel.numInstances, 0, 0, 0);
                }

                commandList.PopMarker();
            }

            commandList.EndPipeline(pipeline);
        }
    };

    renderGraph->AddPass<CModelPassData>("CModel Pass", setup, execute);
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

bool CModelRenderer::LoadCModel(std::string modelPath, u32& objectID)
{
    u32 nameHash = StringUtils::fnv1a_32(modelPath.c_str(), modelPath.length());

    auto it = _nameHashToIndexMap.find(nameHash);
    if (it != _nameHashToIndexMap.end())
    {
        objectID = it->second;
        return true;
    }

    static u32 objectCount = 0;
    NC_LOG_MESSAGE("Loading CModel %u", objectCount);
    objectCount++;

    if (!StringUtils::EndsWith(modelPath, ".cmodel"))
    {
        NC_LOG_FATAL("Tried to call 'LoadCModel' with a reference to a file that didn't end with '.cmodel'");
        return false;
    }

    fs::path cModelPath = "Data/extracted/CModels/" + modelPath;
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

    CModel::ComplexModel cModel;

    if (!cModelBuffer.Get(cModel.header))
        return false;

    if (!cModelBuffer.Get(cModel.flags))
        return false;
    
    // Read Vertices
    {
        u32 numVertices = 0;
        if (!cModelBuffer.GetU32(numVertices))
            return false;

        if (numVertices > 0)
        {
            cModel.vertices.resize(numVertices);
            cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.vertices.data()), numVertices * sizeof(CModel::ComplexVertex));
        }
        else
        {
            return false;
        }
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

    // Read Texture UV Animation Lookup Table
    /*{
        u32 numElements = 0;
        if (!cModelBuffer.GetU32(numElements))
            return false;

        if (numElements > 0)
        {
            cModel.textureUVAnimationLookupTable.resize(numElements);
            cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.textureUVAnimationLookupTable.data()), numElements * sizeof(u16));
        }
    }*/

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

    entt::registry* registry = ServiceLocator::GetGameRegistry();
    TextureSingleton& textureSingleton = registry->ctx<TextureSingleton>();

    u32 nextID = static_cast<u32>(_loadedCModels.size());
    LoadedCModel& loadedCModel = _loadedCModels.emplace_back();
    loadedCModel.debugName = modelPath;

    size_t numRenderBatches = cModel.modelData.renderBatches.size();
    loadedCModel.mesh.renderBatches.resize(numRenderBatches);

    for (u32 i = 0; i < numRenderBatches; i++)
    {
        CModel::ComplexRenderBatch& complexRenderBatch = cModel.modelData.renderBatches[i];
        RenderBatch& renderBatch = loadedCModel.mesh.renderBatches[i];

        renderBatch.indexStart = complexRenderBatch.indexStart;
        renderBatch.indexCount = complexRenderBatch.indexCount;

        // Create Texture Units
        for (u32 j = 0; j < complexRenderBatch.textureUnits.size(); j++)
        {
            CModel::ComplexTextureUnit& complexTextureUnit = complexRenderBatch.textureUnits[j];
            CModel::ComplexMaterial& complexMaterial = cModel.materials[complexTextureUnit.materialIndex];

            if (complexMaterial.flags.disableBackfaceCulling)
                renderBatch.isBackfaceCulled = false;

            renderBatch.textureUnitIndices[j] = static_cast<u8>(loadedCModel.mesh.textureUnits.size());

            TextureUnit& textureUnit = loadedCModel.mesh.textureUnits.emplace_back();
            
            // TODO: Use "isOpaque" to define which pipeline to use
            //textureUnit.isOpaque = complexMaterial.blendingMode == 0 || (complexMaterial.blendingMode == 1 && complexMaterial.flags.depthWrite == 1);
           
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

        // -- Create Index Buffer --
        {
            const size_t bufferSize = renderBatch.indexCount * sizeof(u16);

            Renderer::BufferDesc desc;
            desc.name = "IndexBuffer";
            desc.size = bufferSize;
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_INDEX_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
            desc.cpuAccess = Renderer::BufferCPUAccess::None;

            renderBatch.indexBuffer = _renderer->CreateBuffer(desc);

            // Create staging buffer
            desc.name = "IndexBufferStaging";
            desc.size = bufferSize;
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
            desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

            Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

            // Upload to staging buffer

            void* dst = _renderer->MapBuffer(stagingBuffer);
            memcpy(dst, &cModel.modelData.indices.data()[renderBatch.indexStart], bufferSize);
            _renderer->UnmapBuffer(stagingBuffer);

            // Queue destroy staging buffer
            _renderer->QueueDestroyBuffer(stagingBuffer);

            // Copy from staging buffer to buffer
            _renderer->CopyBuffer(renderBatch.indexBuffer, 0, stagingBuffer, 0, bufferSize);
        }

        // -- Create Texture Unit Indices Buffer --
        {
            const size_t bufferSize = sizeof(renderBatch.textureUnitIndices);

            Renderer::BufferDesc desc;
            desc.name = "TextureUnitIndicesBuffer";
            desc.size = bufferSize;
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_UNIFORM_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
            desc.cpuAccess = Renderer::BufferCPUAccess::None;

            renderBatch.textureUnitIndicesBuffer = _renderer->CreateBuffer(desc);

            // Create staging buffer
            desc.name = "TextureUnitIndicesBufferStaging";
            desc.size = bufferSize;
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
            desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

            Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

            // Upload to staging buffer

            void* dst = _renderer->MapBuffer(stagingBuffer);
            memcpy(dst, &renderBatch.textureUnitIndices[0], bufferSize);
            _renderer->UnmapBuffer(stagingBuffer);

            // Queue destroy staging buffer
            _renderer->QueueDestroyBuffer(stagingBuffer);

            // Copy from staging buffer to buffer
            _renderer->CopyBuffer(renderBatch.textureUnitIndicesBuffer, 0, stagingBuffer, 0, bufferSize);
        }
    }

    // TODO: Write the Vertices in seperate arrays in the extractor
    size_t numVertices = cModel.vertices.size();

    std::vector<vec3> vertexPositions;
    std::vector<vec3> vertexNormals;
    std::vector<vec2> vertexUV0;
    std::vector<vec2> vertexUV1;

    vertexPositions.resize(numVertices);
    vertexNormals.resize(numVertices);
    vertexUV0.resize(numVertices);
    vertexUV1.resize(numVertices);

    for (u32 i = 0; i < numVertices; i++)
    {
        CModel::ComplexVertex& vertex = cModel.vertices[i];

        vertexPositions[i] = vertex.position;
        vertexNormals[i] = vertex.normal;
        vertexUV0[i] = vertex.uvCords[0];
        vertexUV1[i] = vertex.uvCords[1];
    }

    // -- Create Vertex Position Buffer --
    {
        const size_t bufferSize = vertexPositions.size() * sizeof(vec3);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "VertexPositions";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        loadedCModel.mesh.vertexPositionsBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "VertexPositionsStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);

        memcpy(dst, vertexPositions.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(loadedCModel.mesh.vertexPositionsBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create Vertex Normal Buffer --
    {
        const size_t bufferSize = vertexNormals.size() * sizeof(vec3);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "VertexNormals";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        loadedCModel.mesh.vertexNormalsBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "VertexNormalsStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);

        memcpy(dst, vertexNormals.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(loadedCModel.mesh.vertexNormalsBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create UV0 Buffer --
    {
        const size_t bufferSize = vertexUV0.size() * sizeof(vec2);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "VertexUVs0";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        loadedCModel.mesh.vertexUVs0Buffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "VertexUVs0Staging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);

        memcpy(dst, vertexUV0.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(loadedCModel.mesh.vertexUVs0Buffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create UV1 Buffer --
    {
        const size_t bufferSize = vertexUV1.size() * sizeof(vec2);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "VertexUVs1";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        loadedCModel.mesh.vertexUVs1Buffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "VertexUVs1Staging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);

        memcpy(dst, vertexUV1.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(loadedCModel.mesh.vertexUVs1Buffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create Texture Unit Buffer --
    {
        const size_t bufferSize = loadedCModel.mesh.textureUnits.size() * sizeof(TextureUnit);

        Renderer::BufferDesc desc;
        desc.name = "TextureUnitsBuffer";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        loadedCModel.mesh.textureUnitsBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "TextureUnitsBufferStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer

        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, loadedCModel.mesh.textureUnits.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(loadedCModel.mesh.textureUnitsBuffer, 0, stagingBuffer, 0, bufferSize);
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

        loadedCModel.instanceBuffer = _renderer->CreateBuffer(desc);
    }

    objectID = nextID;
    return true;
}

bool CModelRenderer::LoadCreatureCModel(std::string model, DBC::CreatureDisplayInfo* displayInfo, DBC::CreatureModelData* modelData, u32& objectID)
{
    u32 nameHash = StringUtils::fnv1a_32(model.c_str(), model.length());

    auto it = _nameHashToIndexMap.find(nameHash);
    if (it != _nameHashToIndexMap.end())
    {
        objectID = it->second;
        return true;
    }

    static u32 objectCount = 0;
    NC_LOG_MESSAGE("Loading Creature NM2 %u", objectCount);
    objectCount++;

    u32 nextID = static_cast<u32>(_loadedCModels.size());
    LoadedCModel& loadedNM2 = _loadedCModels.emplace_back();
    loadedNM2.debugName = model;

    if (!StringUtils::EndsWith(model, ".cModel"))
    {
        NC_LOG_FATAL("Tried to call 'LoadCreatureCModel' with a reference to a file that didn't end with '.cModel'");
        return false;
    }

    std::string modelTextureFolder = "Data/extracted/Textures/" + fs::path(model).parent_path().string() + "/";
    fs::path absoluteModelPath = "Data/extracted/NM2/" + model;
    absoluteModelPath.make_preferred();
    absoluteModelPath = fs::absolute(absoluteModelPath);

    FileReader cModelFile(absoluteModelPath.string(), absoluteModelPath.filename().string());
    if (!cModelFile.Open())
    {
        NC_LOG_FATAL("Failed to open NM2 file: %s", absoluteModelPath.string().c_str());
        return false;
    }

    Bytebuffer cModelBuffer(nullptr, cModelFile.Length());
    cModelFile.Read(&cModelBuffer, cModelBuffer.size);
    cModelFile.Close();

    /*NM2::NM2Root cModel;

    if (!cModelBuffer.Get(cModel.header))
        return false;

    u32 numVertices = 0;
    if (!cModelBuffer.GetU32(numVertices))
        return false;

    if (numVertices > 0)
    {
        cModel.vertices.resize(numVertices);
        cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.vertices.data()), numVertices * sizeof(NM2::M2Vertex));
    }

    u32 numTextures = 0;
    if (!cModelBuffer.GetU32(numTextures))
        return false;

    if (numTextures > 0)
    {
        cModel.textures.resize(numTextures);
        cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.textures.data()), numTextures * sizeof(NM2::M2Texture));
    }

    u32 numMaterials = 0;
    if (!cModelBuffer.GetU32(numMaterials))
        return false;

    if (numMaterials > 0)
    {
        cModel.materials.resize(numMaterials);
        cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.materials.data()), numMaterials * sizeof(NM2::M2Material));
    }

    u32 numTextureIndicesToId = 0;
    if (!cModelBuffer.GetU32(numTextureIndicesToId))
        return false;

    if (numTextureIndicesToId > 0)
    {
        cModel.textureIndicesToId.resize(numTextureIndicesToId);
        if (!cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.textureIndicesToId.data()), numTextureIndicesToId * sizeof(u16)))
            return false;
    }

    u32 numTextureCombos = 0;
    if (!cModelBuffer.GetU32(numTextureCombos))
        return false;

    if (numTextureCombos > 0)
    {
        cModel.textureCombos.resize(numTextureCombos);
        if (!cModelBuffer.GetBytes(reinterpret_cast<u8*>(cModel.textureCombos.data()), numTextureCombos * sizeof(u16)))
            return false;
    }

    u32 numSkins = 0;
    if (!cModelBuffer.GetU32(numSkins))
        return false;

    if (!numSkins)
        return false;

    std::vector<Material> materials;
    materials.reserve(8);

    // Create mesh, each MapObject becomes one Mesh in loadedMapObject
    Mesh& mesh = loadedCModel.mesh;
    NM2::M2Skin& skin = cModel.skin;
    std::vector<vec3> vertexPositions;
    std::vector<vec3> vertexNormals;
    std::vector<vec2> uv0Coordinates;
    std::vector<vec2> uv1Coordinates;

    if (!cModelBuffer.GetU32(skin.token))
        return false;

    u32 numSkinVertexIndexes = 0;
    if (!cModelBuffer.GetU32(numSkinVertexIndexes))
        return false;

    if (numSkinVertexIndexes > 0)
    {
        skin.vertexIndexes.resize(numSkinVertexIndexes);
        if (!cModelBuffer.GetBytes(reinterpret_cast<u8*>(skin.vertexIndexes.data()), numSkinVertexIndexes * sizeof(u16)))
            return false;
    }

    u32 numSkinIndices = 0;
    if (!cModelBuffer.GetU32(numSkinIndices))
        return false;

    if (numSkinIndices > 0)
    {
        skin.indices.resize(numSkinIndices);
        if (!cModelBuffer.GetBytes(reinterpret_cast<u8*>(skin.indices.data()), numSkinIndices * sizeof(u16)))
            return false;
    }

    u32 numSkinSubMeshes = 0;
    if (!cModelBuffer.GetU32(numSkinSubMeshes))
        return false;

    mesh.subMeshes.resize(numSkinSubMeshes);
    mesh.textureUnits.resize(numSkinSubMeshes);

    vertexPositions.reserve(numSkinVertexIndexes);
    uv0Coordinates.reserve(numSkinVertexIndexes);
    uv1Coordinates.reserve(numSkinVertexIndexes);
    for (const u16& vertexIndex : skin.vertexIndexes)
    {
        const NM2::M2Vertex& vertex = cModel.vertices[vertexIndex];

        vertexPositions.push_back(vertex.position);
        vertexNormals.push_back(vertex.normal);
        uv0Coordinates.push_back(vertex.uvCords[0]);
        uv1Coordinates.push_back(vertex.uvCords[1]);
    }

    for (u32 i = 0; i < numSkinSubMeshes; i++)
    {
        SubMesh& subMesh = mesh.subMeshes[i];

        if (!cModelBuffer.GetU16(subMesh.indexStart))
            return false;

        if (!cModelBuffer.GetU16(subMesh.indexCount))
            return false;

        subMesh.materialNum = i;

        // -- Create Index Buffer --
        {
            const size_t bufferSize = subMesh.indexCount * sizeof(u16);

            Renderer::BufferDesc desc;
            desc.name = "IndexBuffer";
            desc.size = bufferSize;
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_INDEX_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
            desc.cpuAccess = Renderer::BufferCPUAccess::None;

            subMesh.indexBuffer = _renderer->CreateBuffer(desc);

            // Create staging buffer
            desc.name = "IndexBufferStaging";
            desc.size = bufferSize;
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
            desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

            Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

            // Upload to staging buffer

            void* dst = _renderer->MapBuffer(stagingBuffer);
            memcpy(dst, &skin.indices.data()[subMesh.indexStart], bufferSize);
            _renderer->UnmapBuffer(stagingBuffer);

            // Queue destroy staging buffer
            _renderer->QueueDestroyBuffer(stagingBuffer);

            // Copy from staging buffer to buffer
            _renderer->CopyBuffer(subMesh.indexBuffer, 0, stagingBuffer, 0, bufferSize);
        }
    }

    // 1 Material per TextureUnit
    materials.resize(numSkinSubMeshes);

    for (u32 i = 0; i < numSkinSubMeshes; i++)
    {
        TextureUnits& textureUnit = mesh.textureUnits[i];

        if (!cModelBuffer.GetU8(textureUnit.flags))
            return false;

        if (!cModelBuffer.GetU16(textureUnit.shaderId))
            return false;

        if (!cModelBuffer.GetU16(textureUnit.skinSectionIndex))
            return false;

        if (!cModelBuffer.GetU16(textureUnit.geosetIndex))
            return false;

        if (!cModelBuffer.GetU16(textureUnit.materialIndex))
            return false;

        if (!cModelBuffer.GetU16(textureUnit.textureCount))
            return false;

        if (!cModelBuffer.GetU16(textureUnit.textureComboIndex))
            return false;

        Material& material = materials[textureUnit.skinSectionIndex];
        material.type = textureUnit.shaderId;
        material.blendingMode = cModel.materials[textureUnit.materialIndex].blendingMode;

        for (u16 j = 0; j < textureUnit.textureCount; j++)
        {
            // Set TextureId to the offset into cModel.textures (Below we will load all textures and then we will resolve the actual texture id)
            material.textureIDs[j] = cModel.textureCombos[textureUnit.textureComboIndex + j];
        }
    }

    // -- Create Vertex Positions Buffer --
    {
        const size_t bufferSize = vertexPositions.size() * sizeof(vec3);

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

        memcpy(dst, vertexPositions.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(mesh.vertexPositionsBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create Vertex Normals Buffer --
    {
        const size_t bufferSize = vertexNormals.size() * sizeof(vec3);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "VertexNormals";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        mesh.vertexNormalsBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "VertexNormalsStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);

        memcpy(dst, vertexNormals.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(mesh.vertexNormalsBuffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create UV0 Buffer --
    {
        const size_t bufferSize = uv0Coordinates.size() * sizeof(vec2);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "VertexUVs0";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        mesh.vertexUVs0Buffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "VertexUVs0Staging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);

        memcpy(dst, uv0Coordinates.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(mesh.vertexUVs0Buffer, 0, stagingBuffer, 0, bufferSize);
    }

    // -- Create UV1 Buffer --
    {
        const size_t bufferSize = uv1Coordinates.size() * sizeof(vec2);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "VertexUVs1";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        mesh.vertexUVs1Buffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "VertexUVs1Staging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);

        memcpy(dst, uv1Coordinates.data(), bufferSize);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(mesh.vertexUVs1Buffer, 0, stagingBuffer, 0, bufferSize);
    }

    entt::registry* registry = ServiceLocator::GetGameRegistry();
    TextureSingleton& textureSingleton = registry->ctx<TextureSingleton>();
    DBCSingleton& dbcSingleton = registry->ctx<DBCSingleton>();

    // Load all textures into TextureArray
    for (u32 i = 0; i < numTextures; i++)
    {
        const NM2::M2Texture& texture = cModel.textures[i];
        u32& textureId = loadedCModel.textureIds.emplace_back();
        textureId = INVALID_M2_TEXTURE_ID;

        if (texture.textureNameIndex == std::numeric_limits<u32>().max())
            continue;

        if (texture.type == NM2::TextureTypes::DEFAULT)
        {
            Renderer::TextureDesc textureDesc;
            textureDesc.path = "Data/extracted/Textures/" + textureSingleton.textureStringTable.GetString(texture.textureNameIndex);

            _renderer->LoadTextureIntoArray(textureDesc, _cModelTextures, textureId);
        }
        else if (texture.type == NM2::TextureTypes::MONSTER_SKIN_1)
        {
            Renderer::TextureDesc textureDesc;
            textureDesc.path = modelTextureFolder + dbcSingleton.stringTable.GetString(displayInfo->texture1) + ".dds";

            _renderer->LoadTextureIntoArray(textureDesc, _cModelTextures, textureId);
        }
        else if (texture.type == NM2::TextureTypes::MONSTER_SKIN_2)
        {
            Renderer::TextureDesc textureDesc;
            textureDesc.path = modelTextureFolder + dbcSingleton.stringTable.GetString(displayInfo->texture2) + ".dds";

            _renderer->LoadTextureIntoArray(textureDesc, _cModelTextures, textureId);
        }
        else if (texture.type == NM2::TextureTypes::MONSTER_SKIN_3)
        {
            Renderer::TextureDesc textureDesc;
            textureDesc.path = modelTextureFolder + dbcSingleton.stringTable.GetString(displayInfo->texture3) + ".dds";

            _renderer->LoadTextureIntoArray(textureDesc, _cModelTextures, textureId);
        }
        else
        {
            assert(false);
        }
    }

    // Resolve all material texture ids
    for (Material& material : materials)
    {
        for (i32 i = 0; i < 4; i++)
        {
            u32& textureId = material.textureIDs[i];
            if (textureId == INVALID_M2_TEXTURE_ID)
                continue;

            textureId = loadedCModel.textureIds[textureId];
        }
    }

    // -- Create Materials Buffer
    {
        const size_t bufferSize = materials.size() * sizeof(Material);

        // Create buffer
        Renderer::BufferDesc desc;
        desc.name = "Materials";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_DESTINATION;
        desc.cpuAccess = Renderer::BufferCPUAccess::None;

        loadedCModel.materialsBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "MaterialsStaging";
        desc.size = bufferSize;
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, materials.data(), bufferSize);

        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(loadedCModel.materialsBuffer, 0, stagingBuffer, 0, bufferSize);
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

        loadedCModel.instanceBuffer = _renderer->CreateBuffer(desc);
    }*/

    objectID = nextID;
    return true;
}

bool CModelRenderer::LoadCreature(u32 displayId, u32& objectID)
{
    entt::registry* registry = ServiceLocator::GetGameRegistry();
    DBCSingleton& dbcSingleton = registry->ctx<DBCSingleton>();
    DisplayInfoSingleton& displayInfoSingleton = registry->ctx<DisplayInfoSingleton>();

    // Ensure DisplayInfo Exists
    auto displayInfoItr = displayInfoSingleton.creatureDisplayIdToDisplayInfo.find(displayId);
    if (displayInfoItr == displayInfoSingleton.creatureDisplayIdToDisplayInfo.end())
        return false;

    DBC::CreatureDisplayInfo* displayInfo = displayInfoItr->second;

    // Ensure ModelData Exists
    auto modelDataItr = displayInfoSingleton.creatureModelIdToModelData.find(displayInfo->modelId);
    if (modelDataItr == displayInfoSingleton.creatureModelIdToModelData.end())
        return false;

    DBC::CreatureModelData* modelData = modelDataItr->second;

    // Check if ModelPath is valid
    if (modelData->modelPath == std::numeric_limits<u32>().max())
        return false;

    // Get ModelPath String from StringTable
    std::string modelPath = dbcSingleton.stringTable.GetString(modelData->modelPath);

    if (LoadCreatureCModel(modelPath, displayInfo, modelData, objectID))
    {
        LoadedCModel& cModel = _loadedCModels[objectID];

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

        vec3 pos = vec3(-8000, 100.f, 1600.f);
        mat4x4 rotationMatrix = glm::eulerAngleXYZ(glm::radians(0.f), glm::radians(0.f), glm::radians(0.f));

        dst->instanceMatrix = glm::translate(mat4x4(1.0f), pos) * rotationMatrix;
        _renderer->UnmapBuffer(stagingBuffer);

        u32 dstOffset = sizeof(Instance) * cModel.numInstances++;

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);

        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(cModel.instanceBuffer, dstOffset, stagingBuffer, 0, bufferSize);
    }

    return true;
}

void CModelRenderer::LoadFromChunk(const Terrain::Chunk& chunk, StringTable& stringTable)
{
    for (const Terrain::Placement& complexModelPlacement : chunk.complexModelPlacements)
    {
        const std::string& name = stringTable.GetString(complexModelPlacement.nameID);

        u32 objectID = 0;
        if (LoadCModel(name, objectID))
        {
            LoadedCModel& cModel = _loadedCModels[objectID];
            
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

            vec3 pos = complexModelPlacement.position;
            pos = vec3(Terrain::MAP_HALF_SIZE - pos.x, pos.y, Terrain::MAP_HALF_SIZE - pos.z); // Go from [0 .. MAP_SIZE] to [-MAP_HALF_SIZE .. MAP_HALF_SIZE]
            pos = vec3(pos.z, pos.y, pos.x); // Swizzle and invert x and z

            vec3 rot = glm::radians(complexModelPlacement.rotation);
            rot = vec3(rot.z, -rot.y, rot.x);

            vec3 scale = vec3(complexModelPlacement.scale) / 1024.0f;

            mat4x4 rotationMatrix = glm::eulerAngleXYZ(rot.x, rot.y, rot.z);
            mat4x4 scaleMatrix = glm::scale(mat4x4(1.0f), scale);

            dst->instanceMatrix = glm::translate(mat4x4(1.0f), pos) * rotationMatrix;// *scaleMatrix;
            _renderer->UnmapBuffer(stagingBuffer);

            u32 dstOffset = sizeof(Instance) * cModel.numInstances++;

            // Queue destroy staging buffer
            _renderer->QueueDestroyBuffer(stagingBuffer);

            // Copy from staging buffer to buffer
            _renderer->CopyBuffer(cModel.instanceBuffer, dstOffset, stagingBuffer, 0, bufferSize);
        }
    }
}
