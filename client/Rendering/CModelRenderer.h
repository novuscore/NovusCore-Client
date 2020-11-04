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
    CModelRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer);
    ~CModelRenderer();

    void Update(f32 deltaTime);

    void AddComplexModelPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

    void RegisterLoadFromChunk(const Terrain::Chunk& chunk, StringTable& stringTable);
    void ExecuteLoad();

    void Clear();
    
private:
    struct ComplexModelToBeLoaded
    {
        const Terrain::Placement* placement = nullptr;
        const std::string* name = nullptr;
        u32 nameHash = 0;
    };

    struct Instance
    {
        mat4x4 instanceMatrix;
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
        
        u16 textureUnitOffset;
        u16 numTextureUnits;
    };

    static const u32 MAX_INSTANCES = 512;
    struct LoadedComplexModel
    {
        std::string debugName = "";

        u32 numDrawCalls = 0;
        std::vector<DrawCall> drawCallTemplates;
        std::vector<DrawCallData> drawCallDataTemplates;

        u32 numTwoSidedDrawCalls = 0;
        std::vector<DrawCall> twoSidedDrawCallTemplates;
        std::vector<DrawCallData> twoSidedDrawCallDataTemplates;
    };

private:
    void CreatePermanentResources();

    bool LoadComplexModel(ComplexModelToBeLoaded& complexModelToBeLoaded, LoadedComplexModel& complexModel);
    bool LoadFile(const std::string& cModelPathString, CModel::ComplexModel& cModel);

    bool IsRenderBatchTwoSided(const CModel::ComplexRenderBatch& renderBatch, const CModel::ComplexModel& cModel);

    void AddInstance(LoadedComplexModel& complexModel, const Terrain::Placement& placement);

    void CreateBuffers();

private:
    Renderer::Renderer* _renderer;

    Renderer::SamplerID _sampler;
    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _meshDescriptorSet;

    std::vector<ComplexModelToBeLoaded> _complexModelsToBeLoaded;
    std::vector<LoadedComplexModel> _loadedComplexModels;
    robin_hood::unordered_map<u32, u32> _nameHashToIndexMap;

    std::vector<CModel::ComplexVertex> _vertices;
    std::vector<u16> _indices;
    std::vector<TextureUnit> _textureUnits;
    std::vector<Instance> _instances;

    std::vector<DrawCall> _drawCalls;
    std::vector<DrawCallData> _drawCallDatas;

    std::vector<DrawCall> _twoSidedDrawCalls;
    std::vector<DrawCallData> _twoSidedDrawCallDatas;

    Renderer::BufferID _vertexBuffer;
    Renderer::BufferID _indexBuffer;
    Renderer::BufferID _textureUnitBuffer;
    Renderer::BufferID _instanceBuffer;

    Renderer::BufferID _drawCallBuffer;
    Renderer::BufferID _drawCallDataBuffer;

    Renderer::BufferID _twoSidedDrawCallBuffer;
    Renderer::BufferID _twoSidedDrawCallDataBuffer;

    Renderer::TextureArrayID _cModelTextures;

    DebugRenderer* _debugRenderer;
};