#pragma once
#include <NovusTypes.h>

#include <Utils/StringUtils.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/TextureArrayDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Descriptors/BufferDesc.h>
#include <Renderer/Buffer.h>
#include <Renderer/DescriptorSet.h>

#include "../Gameplay/Map/Chunk.h"
#include "CModel/CModel.h"
#include "ViewConstantBuffer.h"

namespace Renderer
{
    class RenderGraph;
    class Renderer;
    class DescriptorSet;
}

namespace Terrain
{
    struct PlacementDetails;
}

namespace NDBC
{
    struct CreatureDisplayInfo;
    struct CreatureModelData;
}

class CameraFreeLook;
class DebugRenderer;
class MapObjectRenderer;

constexpr u32 CMODEL_INVALID_TEXTURE_ID = std::numeric_limits<u32>().max();
constexpr u8 CMODEL_INVALID_TEXTURE_UNIT_INDEX = std::numeric_limits<u8>().max();
class CModelRenderer
{
public:
    struct DrawCall
    {
        u32 indexCount;
        u32 instanceCount;
        u32 firstIndex;
        u32 vertexOffset;
        u32 firstInstance;
    };

    struct DrawCallData
    {
        u32 instanceID;
        u32 cullingDataID;
        u16 textureUnitOffset;
        u16 numTextureUnits;
        u32 renderPriority;
    };

    struct LoadedComplexModel
    {
        u32 objectID;
        std::string debugName = "";

        u32 cullingDataID = std::numeric_limits<u32>().max();

        u32 numOpaqueDrawCalls = 0;
        std::vector<DrawCall> opaqueDrawCallTemplates;
        std::vector<DrawCallData> opaqueDrawCallDataTemplates;

        u32 numTransparentDrawCalls = 0;
        std::vector<DrawCall> transparentDrawCallTemplates;
        std::vector<DrawCallData> transparentDrawCallDataTemplates;
    };

    struct Instance
    {
        mat4x4 instanceMatrix;
    };

public:
    CModelRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer);
    ~CModelRenderer();

    void Update(f32 deltaTime);

    void AddComplexModelPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID colorTarget, Renderer::ImageID objectTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

    void RegisterLoadFromChunk(u16 chunkID, const Terrain::Chunk& chunk, StringTable& stringTable);
    void ExecuteLoad();

    void Clear();

    const std::vector<DrawCallData>& GetOpaqueDrawCallData() { return _opaqueDrawCallDatas; }
    const std::vector<DrawCallData>& GetTransparentDrawCallData() { return _transparentDrawCallDatas; }
    const std::vector<LoadedComplexModel>& GetLoadedComplexModels() { return _loadedComplexModels; }
    const std::vector<Instance>& GetInstances() { return _instances; }
    const std::vector<Terrain::PlacementDetails>& GetPlacementDetails() { return _complexModelPlacementDetails; }
    const std::vector<CModel::CullingData>& GetCullingData() { return _cullingDatas; }

    u32 GetChunkPlacementDetailsOffset(u16 chunkID) { return _mapChunkToPlacementOffset[chunkID]; }
    u32 GetNumLoadedCModels() { return static_cast<u32>(_loadedComplexModels.size()); }
    u32 GetNumCModelPlacements() { return static_cast<u32>(_instances.size()); }
    u32 GetModelIndexByDrawCallDataIndex(u32 index, bool isOpaque)
    {
        u32 modelIndex = std::numeric_limits<u32>().max();

        if (isOpaque)
            modelIndex = _opaqueDrawCallDataIndexToLoadedModelIndex[index];
        else
            modelIndex = _transparentDrawCallDataIndexToLoadedModelIndex[index];

        return modelIndex;
    }

private:
    struct ComplexModelToBeLoaded
    {
        const Terrain::Placement* placement = nullptr;
        const std::string* name = nullptr;
        u32 nameHash = 0;
    };

    struct TextureUnit
    {
        u16 data = 0; // Texture Flag + Material Flag + Material Blending Mode
        u16 materialType = 0; // Shader ID
        u32 textureIds[2] = { CMODEL_INVALID_TEXTURE_ID, CMODEL_INVALID_TEXTURE_ID };
        u32 pad;
    };

    struct RenderBatch
    {
        u16 indexStart = 0;
        u16 indexCount = 0;
        bool isBackfaceCulled = true;

        u8 textureUnitIndices[8] =
        {
            CMODEL_INVALID_TEXTURE_UNIT_INDEX, CMODEL_INVALID_TEXTURE_UNIT_INDEX,
            CMODEL_INVALID_TEXTURE_UNIT_INDEX, CMODEL_INVALID_TEXTURE_UNIT_INDEX,
            CMODEL_INVALID_TEXTURE_UNIT_INDEX, CMODEL_INVALID_TEXTURE_UNIT_INDEX,
            CMODEL_INVALID_TEXTURE_UNIT_INDEX, CMODEL_INVALID_TEXTURE_UNIT_INDEX
        };

        Renderer::BufferID indexBuffer;
        Renderer::BufferID textureUnitIndicesBuffer;
    };

    struct Mesh
    {
        std::vector<RenderBatch> renderBatches;
        std::vector<TextureUnit> textureUnits;

        Renderer::BufferID vertexBuffer;
        Renderer::BufferID textureUnitsBuffer;
    };

    struct CullConstants
    {
        vec4 frustumPlanes[6];
        vec3 cameraPos;
        u32 maxDrawCount;
        bool shouldPrepareSort = false;
    };

private:
    void CreatePermanentResources();

    bool LoadComplexModel(ComplexModelToBeLoaded& complexModelToBeLoaded, LoadedComplexModel& complexModel);
    bool LoadFile(const std::string& cModelPathString, CModel::ComplexModel& cModel);

    bool IsRenderBatchTransparent(const CModel::ComplexRenderBatch& renderBatch, const CModel::ComplexModel& cModel);

    void AddInstance(LoadedComplexModel& complexModel, const Terrain::Placement& placement);

    void CreateBuffers();

private:
    Renderer::Renderer* _renderer;

    Renderer::SamplerID _sampler;
    Renderer::DescriptorSet _cullingDescriptorSet;
    Renderer::DescriptorSet _sortingDescriptorSet;
    Renderer::DescriptorSet _passDescriptorSet;

    robin_hood::unordered_map<u32, u8> _uniqueIdCounter;
    robin_hood::unordered_map<u16, u32> _mapChunkToPlacementOffset;
    std::vector<Terrain::PlacementDetails> _complexModelPlacementDetails;

    std::vector<ComplexModelToBeLoaded> _complexModelsToBeLoaded;
    std::vector<LoadedComplexModel> _loadedComplexModels;
    robin_hood::unordered_map<u32, u32> _nameHashToIndexMap;
    robin_hood::unordered_map<u32, u32> _opaqueDrawCallDataIndexToLoadedModelIndex;
    robin_hood::unordered_map<u32, u32> _transparentDrawCallDataIndexToLoadedModelIndex;

    std::vector<CModel::ComplexVertex> _vertices;
    std::vector<u16> _indices;
    std::vector<TextureUnit> _textureUnits;
    std::vector<Instance> _instances;
    std::vector<CModel::CullingData> _cullingDatas;

    std::vector<DrawCall> _opaqueDrawCalls;
    std::vector<DrawCallData> _opaqueDrawCallDatas;

    std::vector<DrawCall> _transparentDrawCalls;
    std::vector<DrawCallData> _transparentDrawCallDatas;

    Renderer::BufferID _vertexBuffer;
    Renderer::BufferID _indexBuffer;
    Renderer::BufferID _textureUnitBuffer;
    Renderer::BufferID _instanceBuffer;
    Renderer::BufferID _cullingDataBuffer;

    Renderer::BufferID _opaqueDrawCallBuffer;
    Renderer::BufferID _opaqueCulledDrawCallBuffer;
    Renderer::BufferID _opaqueDrawCallDataBuffer;
    Renderer::BufferID _opaqueDrawCountBuffer;

    Renderer::BufferID _transparentDrawCallBuffer;
    Renderer::BufferID _transparentCulledDrawCallBuffer;
    Renderer::BufferID _transparentSortedCulledDrawCallBuffer;
    Renderer::BufferID _transparentDrawCallDataBuffer;
    Renderer::BufferID _transparentDrawCountBuffer;

    Renderer::BufferID _transparentSortKeys;
    Renderer::BufferID _transparentSortValues;

    CullConstants _cullConstants;

    Renderer::TextureArrayID _cModelTextures;

    DebugRenderer* _debugRenderer;
};