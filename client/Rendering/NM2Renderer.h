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

constexpr u32 INVALID_M2_TEXTURE_ID = std::numeric_limits<u32>().max();
class NM2Renderer
{
public:
    NM2Renderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer);
    ~NM2Renderer();

    void Update(f32 deltaTime);

    void AddNM2Pass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

    bool LoadCreature(u32 displayId, u32& objectID);
private:
    void CreatePermanentResources();

    bool LoadNM2(std::string nm2Name, u32& objectID);
    bool LoadCreatureNM2(std::string modelPath, DBC::CreatureDisplayInfo* displayInfo, DBC::CreatureModelData* modelData, u32& objectID);

    struct Material
    {
        u32 type = 0;
        u32 blendingMode = 0;
        u32 textureIDs[4] = { INVALID_M2_TEXTURE_ID, INVALID_M2_TEXTURE_ID, INVALID_M2_TEXTURE_ID, INVALID_M2_TEXTURE_ID };
    };

    struct Instance
    {
        mat4x4 instanceMatrix;
    };

    struct SubMesh
    {
        u16 indexStart = 0;
        u16 indexCount = 0;

        u32 materialNum = 0;
        Renderer::BufferID indexBuffer;
    };

    struct TextureUnits
    {
        u8 flags = 0;
        u16 shaderId = 0;
        u16 skinSectionIndex = 0;
        u16 geosetIndex = 0;
        u16 materialIndex = 0;
        u16 textureCount = 0;
        u16 textureComboIndex = 0;
    };

    struct Mesh
    {
        std::vector<SubMesh> subMeshes;
        std::vector<TextureUnits> textureUnits;

        Renderer::BufferID vertexPositionsBuffer;
        Renderer::BufferID vertexNormalsBuffer;
        Renderer::BufferID vertexUVs0Buffer;
        Renderer::BufferID vertexUVs1Buffer;
    };

    static const u32 MAX_INSTANCES = 256;
    struct LoadedNM2
    {
        std::string debugName = "";

        Mesh mesh;
        std::vector<u32> textureIds;

        u32 numInstances;
        Renderer::BufferID instanceBuffer; // One per instance
        Renderer::BufferID materialsBuffer; // One per instance
    };

    Renderer::Renderer* _renderer;

    Renderer::SamplerID _sampler;
    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _meshDescriptorSet;

    std::vector<LoadedNM2> _loadedNM2s;
    robin_hood::unordered_map<u32, u32> _nameHashToIndexMap;

    Renderer::TextureArrayID _m2Textures;

    bool _debugSubMeshRendering = false;
    size_t _numSubMeshesToRender = 0;
    DebugRenderer* _debugRenderer;
};