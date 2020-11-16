#include "CModelRenderer.h"
#include "DebugRenderer.h"
#include "../Utils/ServiceLocator.h"
#include "../Rendering/CModel/CModel.h"

#include <filesystem>
#include <GLFW/glfw3.h>
#include <tracy/TracyVulkan.hpp>

#include <InputManager.h>
#include <Renderer/Renderer.h>
#include <Utils/FileReader.h>
#include <Utils/ByteBuffer.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "../ECS/Components/Singletons/NDBCSingleton.h"
#include "../ECS/Components/Singletons/TextureSingleton.h"
#include "../ECS/Components/Singletons/DisplayInfoSingleton.h"

#include "Camera.h"
#include "../Gameplay/Map/Map.h"
#include "CVar/CVarSystem.h"
#include <tracy/TracyVulkan.hpp>

namespace fs = std::filesystem;

AutoCVar_Int CVAR_ComplexModelCullingEnabled("complexModels.cullEnable", "enable culling of complex models", 1, CVarFlags::EditCheckbox);
AutoCVar_Int CVAR_ComplexModelSortingEnabled("complexModels.sortEnable", "enable sorting of transparent complex models", 1, CVarFlags::EditCheckbox);
AutoCVar_Int CVAR_ComplexModelLockCullingFrustum("complexModels.lockCullingFrustum", "lock frustrum for complex model culling", 0, CVarFlags::EditCheckbox);
AutoCVar_Int CVAR_ComplexModelDrawBoundingBoxes("complexModels.drawBoundingBoxes", "draw bounding boxes for complex models", 0, CVarFlags::EditCheckbox);

constexpr u32 BITONIC_BLOCK_SIZE = 1024;
const u32 TRANSPOSE_BLOCK_SIZE = 16;
constexpr u32 MATRIX_WIDTH = BITONIC_BLOCK_SIZE;

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
    bool drawBoundingBoxes = CVAR_ComplexModelDrawBoundingBoxes.Get() == 1;
    if (drawBoundingBoxes)
    {
        for (DrawCall& drawCall : _opaqueDrawCalls)
        {
            DrawCallData& drawCallData = _opaqueDrawCallDatas[drawCall.firstInstance];
            u32 instanceID = drawCallData.instanceID;
            u32 cullingDataID = drawCallData.cullingDataID;

            Instance& instance = _instances[instanceID];
            CModel::CullingData& cullingData = _cullingDatas[cullingDataID];

            vec3 center = (cullingData.minBoundingBox + cullingData.maxBoundingBox) * f16(0.5f);
            vec3 extents = vec3(cullingData.maxBoundingBox) - center;

            // transform center
            mat4x4& m = instance.instanceMatrix;
            vec3 transformedCenter = vec3(m * vec4(center, 1.0f));

            // Transform extents (take maximum)
            glm::mat3x3 absMatrix = glm::mat3x3(glm::abs(vec3(m[0])), glm::abs(vec3(m[1])), glm::abs(vec3(m[2])));
            vec3 transformedExtents = absMatrix * extents;

            // Transform to min/max box representation
            vec3 transformedMin = transformedCenter - transformedExtents;
            vec3 transformedMax = transformedCenter + transformedExtents;

            _debugRenderer->DrawAABB3D(transformedMin, transformedMax, 0xff00ffff);
        }

        for (DrawCall& drawCall : _transparentDrawCalls)
        {
            DrawCallData& drawCallData = _transparentDrawCallDatas[drawCall.firstInstance];
            u32 instanceID = drawCallData.instanceID;
            u32 cullingDataID = drawCallData.cullingDataID;

            Instance& instance = _instances[instanceID];
            CModel::CullingData& cullingData = _cullingDatas[cullingDataID];

            vec3 center = (cullingData.minBoundingBox + cullingData.maxBoundingBox) * f16(0.5f);
            vec3 extents = vec3(cullingData.maxBoundingBox) - center;

            // transform center
            mat4x4& m = instance.instanceMatrix;
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

void CModelRenderer::AddComplexModelPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
    struct CModelPassData
    {
        Renderer::RenderPassMutableResource mainColor;
        Renderer::RenderPassMutableResource mainDepth;
    };

    const bool cullingEnabled = CVAR_ComplexModelCullingEnabled.Get();
    const bool alphaSortEnabled = CVAR_ComplexModelSortingEnabled.Get();
    const bool lockFrustum = CVAR_ComplexModelLockCullingFrustum.Get();

    renderGraph->AddPass<CModelPassData>("CModel Pass", 
        [=](CModelPassData& data, Renderer::RenderGraphBuilder& builder)
        {
            data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
            data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
        [=](CModelPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList)
        {
            GPU_SCOPED_PROFILER_ZONE(commandList, NM2Pass);

            Renderer::ComputePipelineDesc cullingPipelineDesc;
            resources.InitializePipelineDesc(cullingPipelineDesc);

            Renderer::ComputeShaderDesc shaderDesc;
            shaderDesc.path = "Data/shaders/cModelCulling.cs.hlsl.spv";
            cullingPipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);

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

            // Render targets
            pipelineDesc.renderTargets[0] = data.mainColor;
            pipelineDesc.depthStencil = data.mainDepth;

            // Set Opaque Pipeline
            u32 numOpaqueDrawCalls = static_cast<u32>(_opaqueDrawCalls.size());
            if (numOpaqueDrawCalls > 0)
            {
                commandList.PushMarker("Opaque " + std::to_string(numOpaqueDrawCalls), Color::White);

                // Cull
                if (cullingEnabled)
                {
                    // Reset the counter
                    commandList.FillBuffer(_opaqueDrawCountBuffer, 0, 4, 0);
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, _opaqueDrawCountBuffer);

                    // Do culling
                    Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(cullingPipelineDesc);
                    commandList.BeginPipeline(pipeline);

                    if (!lockFrustum)
                    {
                        Camera* camera = ServiceLocator::GetCamera();
                        memcpy(_opaqueCullConstantBuffer->resource.frustumPlanes, camera->GetFrustumPlanes(), sizeof(vec4[6]));
                        _opaqueCullConstantBuffer->resource.cameraPos = camera->GetPosition();
                        _opaqueCullConstantBuffer->resource.maxDrawCount = numOpaqueDrawCalls;
                        _opaqueCullConstantBuffer->Apply(frameIndex);
                    }

                    _cullingDescriptorSet.Bind("_drawCallDatas", _opaqueDrawCallDataBuffer);
                    _cullingDescriptorSet.Bind("_drawCalls", _opaqueDrawCallBuffer);
                    _cullingDescriptorSet.Bind("_culledDrawCalls", _opaqueCulledDrawCallBuffer);
                    _cullingDescriptorSet.Bind("_drawCount", _opaqueDrawCountBuffer);
                    _cullingDescriptorSet.Bind("_instances", _instanceBuffer);
                    _cullingDescriptorSet.Bind("_cullingDatas", _cullingDataBuffer);
                    _cullingDescriptorSet.Bind("_constants", _opaqueCullConstantBuffer->GetBuffer(frameIndex));
                    
                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_cullingDescriptorSet, frameIndex);
                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

                    commandList.Dispatch((numOpaqueDrawCalls + 31) / 32, 1, 1);

                    commandList.EndPipeline(pipeline);

                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _opaqueCulledDrawCallBuffer);
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _opaqueDrawCountBuffer);
                }
                else
                {
                    // Reset the counter
                    commandList.FillBuffer(_opaqueDrawCountBuffer, 0, 4, numOpaqueDrawCalls);
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToIndirectArguments, _opaqueDrawCountBuffer);
                }

                // Draw
                Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
                commandList.BeginPipeline(pipeline);

                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

                _passDescriptorSet.Bind("_drawCallDatas", _opaqueDrawCallDataBuffer);
                _passDescriptorSet.Bind("_vertices", _vertexBuffer);
                _passDescriptorSet.Bind("_textures", _cModelTextures);
                _passDescriptorSet.Bind("_textureUnits", _textureUnitBuffer);
                _passDescriptorSet.Bind("_instances", _instanceBuffer);
                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

                commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);

                Renderer::BufferID argumentBuffer = (cullingEnabled) ? _opaqueCulledDrawCallBuffer : _opaqueDrawCallBuffer;
                commandList.DrawIndexedIndirectCount(argumentBuffer, 0, _opaqueDrawCountBuffer, 0, numOpaqueDrawCalls);

                commandList.EndPipeline(pipeline);
                commandList.PopMarker();
            }

            // Set Transparent Pipeline
            u32 numTransparentDrawCalls = static_cast<u32>(_transparentDrawCalls.size());
            if (numTransparentDrawCalls > 0)
            {
                commandList.PushMarker("Transparent " + std::to_string(numTransparentDrawCalls), Color::White);

                // Copy _transparentDrawCallBuffer into _transparentCulledDrawCallBuffer
                u32 copySize = numTransparentDrawCalls * sizeof(DrawCall);
                commandList.CopyBuffer(_transparentCulledDrawCallBuffer, 0, _transparentDrawCallBuffer, 0, copySize);

                // Cull
                if (cullingEnabled)
                {
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, _transparentCulledDrawCallBuffer);

                    // Reset the counter
                    commandList.FillBuffer(_transparentDrawCountBuffer, 0, 4, 0);
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, _transparentDrawCountBuffer);

                    // Do culling
                    Renderer::ComputeShaderDesc shaderDesc;
                    shaderDesc.path = "Data/shaders/cModelCullingAlpha.cs.hlsl.spv";
                    cullingPipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);

                    Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(cullingPipelineDesc);
                    commandList.BeginPipeline(pipeline);

                    if (!lockFrustum)
                    {
                        Camera* camera = ServiceLocator::GetCamera();
                        memcpy(_transparentCullConstantBuffer->resource.frustumPlanes, camera->GetFrustumPlanes(), sizeof(vec4[6]));
                        _transparentCullConstantBuffer->resource.cameraPos = camera->GetPosition();
                        _transparentCullConstantBuffer->resource.maxDrawCount = numTransparentDrawCalls;
                        _transparentCullConstantBuffer->Apply(frameIndex);
                    }

                    _cullingDescriptorSet.Bind("_drawCallDatas", _transparentDrawCallDataBuffer);
                    _cullingDescriptorSet.Bind("_drawCalls", _transparentCulledDrawCallBuffer);
                    _cullingDescriptorSet.Bind("_drawCount", _transparentDrawCountBuffer);
                    _cullingDescriptorSet.Bind("_instances", _instanceBuffer);
                    _cullingDescriptorSet.Bind("_cullingDatas", _cullingDataBuffer);
                    _cullingDescriptorSet.Bind("_constants", _transparentCullConstantBuffer->GetBuffer(frameIndex));

                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_cullingDescriptorSet, frameIndex);
                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

                    commandList.Dispatch((numTransparentDrawCalls + 31) / 32, 1, 1);

                    commandList.EndPipeline(pipeline);
                }
                else
                {
                    // Reset the counter
                    commandList.FillBuffer(_transparentDrawCountBuffer, 0, 4, numTransparentDrawCalls);
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToIndirectArguments, _transparentDrawCountBuffer);
                }

                // Sort
                if (alphaSortEnabled)
                {
                    commandList.PushMarker("Sort", Color::White);

                    // Barriers if needed
                    if (cullingEnabled)
                    {
                        commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, _transparentCulledDrawCallBuffer);
                    }
                    else
                    {
                        commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, _transparentCulledDrawCallBuffer);
                    }

                    const u32 width = MATRIX_WIDTH;
                    const u32 height = (numTransparentDrawCalls + MATRIX_WIDTH - 1) / MATRIX_WIDTH;
                    
                    Camera* camera = ServiceLocator::GetCamera();
                    vec3 cameraPosition = camera->GetPosition();
                    u32 numElements = numTransparentDrawCalls;
                    u32 numElementGroups = (numElements + BITONIC_BLOCK_SIZE - 1) / BITONIC_BLOCK_SIZE;
                    
                    for (u32 level = 2; level <= BITONIC_BLOCK_SIZE; level = level * 2)
                    {
                        SortParams sortParams;
                        sortParams.refPosition = cameraPosition;
                        sortParams.level = level;
                        sortParams.levelMask = level;
                        sortParams.width = width;
                        sortParams.height = height;
                        sortParams.dispatchX = 1;
                        sortParams.dispatchY = numElementGroups;
                        sortParams.dispatchZ = 1;
                        
                        SortDrawCalls(resources, commandList, frameIndex, sortParams);
                    }
                    
                    // Then sort the rows and columns for the levels bigger than the block size
                    // Transpose. Sort the Columns. Transpose. Sort the Rows
                    for (u32 level = (BITONIC_BLOCK_SIZE * 2); level <= numElements; level = level * 2)
                    {
                        SortParams sortParams;
                        sortParams.refPosition = cameraPosition;
                        sortParams.level = level;
                        sortParams.levelMask = level;
                        sortParams.width = width;
                        sortParams.height = height;
                        sortParams.dispatchX = 1;
                        sortParams.dispatchY = numElementGroups;
                        sortParams.dispatchZ = 1;
                        
                        {
                            TransposeParams transposeParams;
                            transposeParams.level = level / BITONIC_BLOCK_SIZE;
                            transposeParams.levelMask = (level & ~numElements) / BITONIC_BLOCK_SIZE;
                            transposeParams.height = width;
                            transposeParams.width = height;
                            transposeParams.dispatchX = (height + TRANSPOSE_BLOCK_SIZE - 1) / TRANSPOSE_BLOCK_SIZE;
                            transposeParams.dispatchY = width / TRANSPOSE_BLOCK_SIZE;
                            transposeParams.dispatchZ = 1;
                            
                            TransposeDrawCalls(resources, commandList, frameIndex, transposeParams);
                        }
                        
                        SortDrawCalls(resources, commandList, frameIndex, sortParams);
                        
                        {
                            TransposeParams transposeParams;
                            transposeParams.level = BITONIC_BLOCK_SIZE;
                            transposeParams.levelMask = level;
                            transposeParams.height = width;
                            transposeParams.width = height;
                            transposeParams.dispatchX = (height + TRANSPOSE_BLOCK_SIZE - 1) / TRANSPOSE_BLOCK_SIZE;
                            transposeParams.dispatchY = width / TRANSPOSE_BLOCK_SIZE;
                            transposeParams.dispatchZ = 1;
                            
                            TransposeDrawCalls(resources, commandList, frameIndex, transposeParams);
                        }
                        
                        SortDrawCalls(resources, commandList, frameIndex, sortParams);
                    }
                    commandList.PopMarker();
                }

                // Draw
                if (cullingEnabled || alphaSortEnabled)
                {
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _transparentCulledDrawCallBuffer);
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _transparentDrawCountBuffer);
                }
                else
                {
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToIndirectArguments, _transparentCulledDrawCallBuffer);
                    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToIndirectArguments, _transparentDrawCountBuffer);
                }

                pipelineDesc.states.depthStencilState.depthWriteEnable = false;
                pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
                pipelineDesc.states.blendState.renderTargets[0].blendOp = Renderer::BlendOp::BLEND_OP_ADD;
                //pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::BLEND_MODE_SRC_ALPHA;
                //pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::BLEND_MODE_INV_SRC_ALPHA;
                pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::BLEND_MODE_SRC_ALPHA;
                pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::BLEND_MODE_ONE;

                Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
                commandList.BeginPipeline(pipeline);

                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);

                _passDescriptorSet.Bind("_drawCallDatas", _transparentDrawCallDataBuffer);
                _passDescriptorSet.Bind("_vertices", _vertexBuffer);
                _passDescriptorSet.Bind("_textures", _cModelTextures);
                _passDescriptorSet.Bind("_textureUnits", _textureUnitBuffer);
                _passDescriptorSet.Bind("_instances", _instanceBuffer);
                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

                commandList.SetIndexBuffer(_indexBuffer, Renderer::IndexFormat::UInt16);

                commandList.DrawIndexedIndirect(_transparentCulledDrawCallBuffer, 0, numTransparentDrawCalls);

                commandList.EndPipeline(pipeline);
                commandList.PopMarker();
            }
        });
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
    size_t numComplexModelsToLoad = _complexModelsToBeLoaded.size();

    if (numComplexModelsToLoad == 0)
        return;

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
    _cullingDatas.clear();

    _opaqueDrawCalls.clear();
    _opaqueDrawCallDatas.clear();

    _transparentDrawCalls.clear();
    _transparentDrawCallDatas.clear();

    _renderer->UnloadTexturesInArray(_cModelTextures, 0);
}

void CModelRenderer::SortDrawCalls(Renderer::RenderGraphResources& resources, Renderer::CommandList & commandList, u32 frameIndex, const SortParams & sortParams)
{
    commandList.PushMarker("Sort " + std::to_string(sortParams.level), Color::White);
    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, _transparentCulledDrawCallBuffer);
    
    Renderer::ComputePipelineDesc pipelineDesc;
    resources.InitializePipelineDesc(pipelineDesc);
    
    Renderer::ComputeShaderDesc shaderDesc;
    shaderDesc.path = "Data/shaders/cModelSorting.cs.hlsl.spv";
    pipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);
    
    Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(pipelineDesc);
    commandList.BeginPipeline(pipeline);
    
    SortConstants* constants = resources.FrameNew<SortConstants>();
    constants->referencePosition = sortParams.refPosition;
    constants->level = sortParams.level;
    constants->levelMask = sortParams.levelMask;
    constants->width = sortParams.width;
    constants->height = sortParams.height;
    commandList.PushConstant(constants, 0, sizeof(SortConstants));
    
    _sortingDescriptorSet.Bind("_drawCallDatas", _transparentDrawCallDataBuffer);
    _sortingDescriptorSet.Bind("_drawCalls", _transparentCulledDrawCallBuffer);
    _sortingDescriptorSet.Bind("_input", _transparentCulledDrawCallBuffer2);
    _sortingDescriptorSet.Bind("_instances", _instanceBuffer);
    
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_sortingDescriptorSet, frameIndex);
    
    commandList.Dispatch(sortParams.dispatchX, sortParams.dispatchY, sortParams.dispatchZ);
    
    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}

void CModelRenderer::TransposeDrawCalls(Renderer::RenderGraphResources & resources, Renderer::CommandList & commandList, u32 frameIndex, const TransposeParams & transposeParams)
 {
    commandList.PushMarker("Transpose " + std::to_string(transposeParams.level), Color::White);
    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, _transparentCulledDrawCallBuffer);
    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, _transparentCulledDrawCallBuffer2);
    
    Renderer::ComputePipelineDesc pipelineDesc;
    resources.InitializePipelineDesc(pipelineDesc);
    
    Renderer::ComputeShaderDesc shaderDesc;
    shaderDesc.path = "Data/shaders/cModelTransposing.cs.hlsl.spv";
    pipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);
    
    Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(pipelineDesc);
    commandList.BeginPipeline(pipeline);
    
    TransposeConstants* constants = resources.FrameNew<TransposeConstants>();
    constants->level = transposeParams.level;
    constants->levelMask = transposeParams.levelMask;
    constants->width = transposeParams.width;
    constants->height = transposeParams.height;
    commandList.PushConstant(constants, 0, sizeof(SortConstants));
    
    _sortingDescriptorSet.Bind("_drawCalls", _transparentCulledDrawCallBuffer);
    _sortingDescriptorSet.Bind("_input", _transparentCulledDrawCallBuffer2);
    
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_sortingDescriptorSet, frameIndex);
    
    commandList.Dispatch(transposeParams.dispatchX, transposeParams.dispatchY, transposeParams.dispatchZ);
    
    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
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

    _cullingDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());

    _passDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _passDescriptorSet.Bind("_sampler", _sampler);

    _opaqueCullConstantBuffer = new Renderer::Buffer<CullConstants>(_renderer, "CModelOpaqueCullingConstantBuffer", Renderer::BUFFER_USAGE_UNIFORM_BUFFER, Renderer::BufferCPUAccess::WriteOnly);
    _transparentCullConstantBuffer = new Renderer::Buffer<CullConstants>(_renderer, "CModelTransparentCullingConstantBuffer", Renderer::BUFFER_USAGE_UNIFORM_BUFFER, Renderer::BufferCPUAccess::WriteOnly);

    // Create OpaqueDrawCountBuffer
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelOpaqueDrawCountBuffer";
        desc.size = sizeof(u32);
        desc.usage = Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER | Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _opaqueDrawCountBuffer = _renderer->CreateBuffer(desc);
    }

    // Create TransparentDrawCountBuffer
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelTransparentDrawCountBuffer";
        desc.size = sizeof(u32);
        desc.usage = Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER | Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _transparentDrawCountBuffer = _renderer->CreateBuffer(desc);
    }

    /*ComplexModelToBeLoaded& modelToBeLoaded = _complexModelsToBeLoaded.emplace_back();
    modelToBeLoaded.placement = new Terrain::Placement();
    modelToBeLoaded.name = new std::string("Environments/Stars/IceCrownCitadelSky.cmodel");
    modelToBeLoaded.nameHash = 1337;
    ExecuteLoad();*/
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

    // Handle the CullingData
    size_t numCullingDataBeforeAdd = _cullingDatas.size();

    CModel::CullingData& cullingData = _cullingDatas.emplace_back();
    cullingData = cModel.cullingData;

    // Handle this models renderbatches
    size_t numRenderBatches = static_cast<u32>(cModel.modelData.renderBatches.size());
    for (size_t i = 0; i < numRenderBatches; i++)
    {
        CModel::ComplexRenderBatch& renderBatch = cModel.modelData.renderBatches[i];

        // Select where to store the DrawCall templates, this won't be necessary once we do backface culling in the culling compute shader
        bool isTransparent = IsRenderBatchTransparent(renderBatch, cModel);
        std::vector<DrawCall>& drawCallTemplates = (isTransparent) ? complexModel.transparentDrawCallTemplates : complexModel.opaqueDrawCallTemplates;
        std::vector<DrawCallData>& drawCallDataTemplates = (isTransparent) ? complexModel.transparentDrawCallDataTemplates : complexModel.opaqueDrawCallDataTemplates;

        if (isTransparent)
        {
            complexModel.numTransparentDrawCalls++;
        }
        else
        {
            complexModel.numOpaqueDrawCalls++;
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

                    if (complexTexture.type != CModel::ComplexTextureType::NONE)
                        continue;

                    Renderer::TextureDesc textureDesc;
                    textureDesc.path = textureSingleton.textureHashToPath[complexTexture.textureNameIndex];
                    _renderer->LoadTextureIntoArray(textureDesc, _cModelTextures, textureUnit.textureIds[t]);
                }
            }
        }

        drawCallDataTemplate.cullingDataID = static_cast<u32>(numCullingDataBeforeAdd);
        drawCallDataTemplate.textureUnitOffset = static_cast<u16>(numTextureUnitsBeforeAdd);
        drawCallDataTemplate.numTextureUnits = static_cast<u16>(numTextureUnitsToAdd);
        drawCallDataTemplate.renderPriority = renderBatch.renderPriority;
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

    if (cModel.header.typeID != CModel::COMPLEX_MODEL_TOKEN)
    {
        NC_LOG_FATAL("We opened ComplexModel file (%s) with invalid token %u instead of expected token %u", cModelPath.string().c_str(), cModel.header.typeID, CModel::COMPLEX_MODEL_TOKEN);
    }

    if (cModel.header.typeVersion != CModel::COMPLEX_MODEL_VERSION)
    {
        if (cModel.header.typeVersion < CModel::COMPLEX_MODEL_VERSION)
        {
            NC_LOG_FATAL("Loaded ComplexModel file (%s) with too old version %u instead of expected version of %u, rerun dataextractor", cModelPath.string().c_str(), cModel.header.typeVersion, CModel::COMPLEX_MODEL_VERSION);
        }
        else
        {
            NC_LOG_FATAL("Loaded ComplexModel file (%s) with too new version %u instead of expected version of %u, update your client", cModelPath.string().c_str(), cModel.header.typeVersion, CModel::COMPLEX_MODEL_VERSION);
        }
    }

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

    // Read Culling Data
    if (!cModelBuffer.GetBytes(reinterpret_cast<u8*>(&cModel.cullingData), sizeof(CModel::CullingData)))
        return false;

    return true;
}

bool CModelRenderer::IsRenderBatchTransparent(const CModel::ComplexRenderBatch& renderBatch, const CModel::ComplexModel& cModel)
{
    if (renderBatch.textureUnits.size() > 0)
    {
        const CModel::ComplexMaterial& complexMaterial = cModel.materials[renderBatch.textureUnits[0].materialIndex];
        
        return complexMaterial.blendingMode != 0 && complexMaterial.blendingMode != 1;
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

    // Add the opaque DrawCalls and DrawCallDatas
    size_t numOpaqueDrawCallsBeforeAdd = _opaqueDrawCalls.size();
    for (u32 i = 0; i < complexModel.numOpaqueDrawCalls; i++)
    {
        const DrawCall& drawCallTemplate = complexModel.opaqueDrawCallTemplates[i];
        const DrawCallData& drawCallDataTemplate = complexModel.opaqueDrawCallDataTemplates[i];

        DrawCall& drawCall = _opaqueDrawCalls.emplace_back();
        DrawCallData& drawCallData = _opaqueDrawCallDatas.emplace_back();

        // Copy data from the templates
        drawCall.firstIndex = drawCallTemplate.firstIndex;
        drawCall.indexCount = drawCallTemplate.indexCount;
        drawCall.instanceCount = drawCallTemplate.instanceCount;
        drawCall.vertexOffset = drawCallTemplate.vertexOffset;

        drawCallData.textureUnitOffset = drawCallDataTemplate.textureUnitOffset;
        drawCallData.numTextureUnits = drawCallDataTemplate.numTextureUnits;
        
        // Fill in the data that shouldn't be templated
        drawCall.firstInstance = static_cast<u32>(numOpaqueDrawCallsBeforeAdd + i); // This is used in the shader to retrieve the DrawCallData
        drawCallData.instanceID = static_cast<u32>(numInstancesBeforeAdd);
    }

    // Add the transparent DrawCalls and DrawCallDatas
    size_t numTransparentDrawCallsBeforeAdd = _transparentDrawCalls.size();
    for (u32 i = 0; i < complexModel.numTransparentDrawCalls; i++)
    {
        const DrawCall& drawCallTemplate = complexModel.transparentDrawCallTemplates[i];
        const DrawCallData& drawCallDataTemplate = complexModel.transparentDrawCallDataTemplates[i];

        DrawCall& drawCall = _transparentDrawCalls.emplace_back();
        DrawCallData& drawCallData = _transparentDrawCallDatas.emplace_back();

        // Copy data from the templates
        drawCall.firstIndex = drawCallTemplate.firstIndex;
        drawCall.indexCount = drawCallTemplate.indexCount;
        drawCall.instanceCount = drawCallTemplate.instanceCount;
        drawCall.vertexOffset = drawCallTemplate.vertexOffset;

        drawCallData.textureUnitOffset = drawCallDataTemplate.textureUnitOffset;
        drawCallData.numTextureUnits = drawCallDataTemplate.numTextureUnits;

        // Fill in the data that shouldn't be templated
        drawCall.firstInstance = static_cast<u32>(numTransparentDrawCallsBeforeAdd + i); // This is used in the shader to retrieve the DrawCallData
        drawCallData.instanceID = static_cast<u32>(numInstancesBeforeAdd);
    }
}

int RoundUp(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
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

    // Create CullingData buffer
    if (_cullingDataBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_cullingDataBuffer);
    }
    {
        Renderer::BufferDesc desc;
        desc.name = "CModelCullDataBuffer";
        desc.size = sizeof(CModel::CullingData) * _cullingDatas.size();
        desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
        _cullingDataBuffer = _renderer->CreateBuffer(desc);

        // Create staging buffer
        desc.name = "CModelCullDataStaging";
        desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

        Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = _renderer->MapBuffer(stagingBuffer);
        memcpy(dst, _cullingDatas.data(), desc.size);
        _renderer->UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        _renderer->QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        _renderer->CopyBuffer(_cullingDataBuffer, 0, stagingBuffer, 0, desc.size);
    }

    // Destroy OpaqueDrawCall buffer
    if (_opaqueDrawCallBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_opaqueDrawCallBuffer);
    }

    // Destroy OpaqueCulledDrawCall buffer
    if (_opaqueCulledDrawCallBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_opaqueCulledDrawCallBuffer);
    }

    if (_opaqueDrawCalls.size() > 0)
    {
        // Create OpaqueDrawCall and OpaqueCulledDrawCall buffer
        {
            Renderer::BufferDesc desc;
            desc.name = "CModelOpaqueDrawCallBuffer";
            desc.size = sizeof(DrawCall) * _opaqueDrawCalls.size();
            desc.usage = Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER | Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
            _opaqueDrawCallBuffer = _renderer->CreateBuffer(desc);
            _opaqueCulledDrawCallBuffer = _renderer->CreateBuffer(desc);

            // Create staging buffer
            desc.name = "CModelOpaqueDrawCallStaging";
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
            desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

            Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

            // Upload to staging buffer
            void* dst = _renderer->MapBuffer(stagingBuffer);
            memcpy(dst, _opaqueDrawCalls.data(), desc.size);
            _renderer->UnmapBuffer(stagingBuffer);

            // Queue destroy staging buffer
            _renderer->QueueDestroyBuffer(stagingBuffer);
            // Copy from staging buffer to buffer
            _renderer->CopyBuffer(_opaqueDrawCallBuffer, 0, stagingBuffer, 0, desc.size);
        }

        // Destroy OpaqueDrawCallData buffer
        if (_opaqueDrawCallDataBuffer != Renderer::BufferID::Invalid())
        {
            _renderer->QueueDestroyBuffer(_opaqueDrawCallDataBuffer);
        }

        // Create OpaqueDrawCallData buffer
        {
            Renderer::BufferDesc desc;
            desc.name = "CModelOpaqueDrawCallDataBuffer";
            desc.size = sizeof(DrawCallData) * _opaqueDrawCallDatas.size();
            desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
            _opaqueDrawCallDataBuffer = _renderer->CreateBuffer(desc);

            // Create staging buffer
            desc.name = "CModelOpaqueDrawCallDataStaging";
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
            desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

            Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

            // Upload to staging buffer
            void* dst = _renderer->MapBuffer(stagingBuffer);
            memcpy(dst, _opaqueDrawCallDatas.data(), desc.size);
            _renderer->UnmapBuffer(stagingBuffer);

            // Queue destroy staging buffer
            _renderer->QueueDestroyBuffer(stagingBuffer);
            // Copy from staging buffer to buffer
            _renderer->CopyBuffer(_opaqueDrawCallDataBuffer, 0, stagingBuffer, 0, desc.size);
        }
    }

    // Destroy TransparentDrawCall buffer
    if (_transparentDrawCallBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_transparentDrawCallBuffer);
    }

    // Destroy TransparentCulledDrawCall buffer
    if (_transparentCulledDrawCallBuffer != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_transparentCulledDrawCallBuffer);
    }

    // Destroy TransparentCulledDrawCall2 buffer
    if (_transparentCulledDrawCallBuffer2 != Renderer::BufferID::Invalid())
    {
        _renderer->QueueDestroyBuffer(_transparentCulledDrawCallBuffer2);
    }

    if (_transparentDrawCalls.size() > 0)
    {
        // Create TransparentDrawCall and TransparentCulledDrawCall buffer
        {
            u32 numDrawCalls = static_cast<u32>(_transparentDrawCalls.size());
            u32 numPaddedDrawCalls = RoundUp(numDrawCalls, BITONIC_BLOCK_SIZE); // To sort we need to have our drawcalls be a multiple of BITONIC_BLOCK_SIZE

            u32 actualSize = sizeof(DrawCall) * numDrawCalls;
            u32 paddedSize = sizeof(DrawCall) * numPaddedDrawCalls;
            
            Renderer::BufferDesc desc;
            desc.name = "CModelAlphaDrawCallBuffer";
            desc.size = paddedSize;
            desc.usage = Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER | Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
            _transparentCulledDrawCallBuffer = _renderer->CreateBuffer(desc);
            _transparentCulledDrawCallBuffer2 = _renderer->CreateBuffer(desc);

            desc.usage |= Renderer::BUFFER_USAGE_TRANSFER_SOURCE;
            _transparentDrawCallBuffer = _renderer->CreateBuffer(desc);

            // Create staging buffer
            desc.name = "CModelAlphaDrawCallStaging";
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
            desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

            Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

            // Upload to staging buffer
            void* dst = _renderer->MapBuffer(stagingBuffer);
            memset(dst, 0, paddedSize);
            memcpy(dst, _transparentDrawCalls.data(), actualSize);
            _renderer->UnmapBuffer(stagingBuffer);

            // Copy from staging buffer to buffer
            _renderer->CopyBuffer(_transparentDrawCallBuffer, 0, stagingBuffer, 0, desc.size);
            // Queue destroy staging buffer
            _renderer->QueueDestroyBuffer(stagingBuffer);
        }

        // Destroy TransparentDrawCallData buffer
        if (_transparentDrawCallDataBuffer != Renderer::BufferID::Invalid())
        {
            _renderer->QueueDestroyBuffer(_transparentDrawCallDataBuffer);
        }

        // Create TransparentDrawCallData buffer
        {
            Renderer::BufferDesc desc;
            desc.name = "CModelAlphaDrawCallDataBuffer";
            desc.size = sizeof(DrawCallData) * _transparentDrawCallDatas.size();
            desc.usage = Renderer::BUFFER_USAGE_STORAGE_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
            _transparentDrawCallDataBuffer = _renderer->CreateBuffer(desc);

            // Create staging buffer
            desc.name = "CModelAlphaDrawCallDataStaging";
            desc.usage = Renderer::BufferUsage::BUFFER_USAGE_TRANSFER_SOURCE;
            desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

            Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(desc);

            // Upload to staging buffer
            void* dst = _renderer->MapBuffer(stagingBuffer);
            memcpy(dst, _transparentDrawCallDatas.data(), desc.size);
            _renderer->UnmapBuffer(stagingBuffer);

            // Queue destroy staging buffer
            _renderer->QueueDestroyBuffer(stagingBuffer);
            // Copy from staging buffer to buffer
            _renderer->CopyBuffer(_transparentDrawCallDataBuffer, 0, stagingBuffer, 0, desc.size);
        }
    }
}
