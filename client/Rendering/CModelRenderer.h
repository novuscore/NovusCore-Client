#pragma once
#include <NovusTypes.h>

#include <array>

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
#include "ViewConstantBuffer.h"

namespace Renderer
{
    class RenderGraph;
    class Renderer;
    class DescriptorSet;
}

namespace DBC
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

    void AddCModelPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

    bool LoadCreature(u32 displayId, u32& objectID);
private:
    void CreatePermanentResources();

    bool LoadCModel(std::string modelPath, u32& objectID);
    bool LoadCreatureCModel(std::string modelPath, DBC::CreatureDisplayInfo* displayInfo, DBC::CreatureModelData* modelData, u32& objectID);

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

        Renderer::BufferID vertexPositionsBuffer;
        Renderer::BufferID vertexNormalsBuffer;
        Renderer::BufferID vertexUVs0Buffer;
        Renderer::BufferID vertexUVs1Buffer;
        Renderer::BufferID textureUnitsBuffer;
    };

    static const u32 MAX_INSTANCES = 256;
    struct LoadedCModel
    {
        std::string debugName = "";

        Mesh mesh;

        u32 numInstances;
        Renderer::BufferID instanceBuffer;
    };

    Renderer::Renderer* _renderer;

    Renderer::SamplerID _sampler;
    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _meshDescriptorSet;

    std::vector<LoadedCModel> _loadedCModels;
    robin_hood::unordered_map<u32, u32> _nameHashToIndexMap;

    Renderer::TextureArrayID _cModelTextures;

    DebugRenderer* _debugRenderer;
};