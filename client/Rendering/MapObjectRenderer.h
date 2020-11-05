#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>
#include <filesystem>
#include <Utils/ByteBuffer.h>

#include <Renderer/Buffer.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/BufferDesc.h>

#include "ViewConstantBuffer.h"
#include "../Gameplay/Map/MapObject.h"

namespace Renderer
{
    class RenderGraph;
    class Renderer;
    class DescriptorSet;
}

namespace Terrain
{
    struct Chunk;
    struct MapObject;
    struct Placement;
}

class StringTable;
class DebugRenderer;

class MapObjectRenderer
{
    struct MeshRoot
    {
        u32 numMaterials;
        u32 numMeshes;
    };

    struct RenderBatch
    {
        u32 firstIndex;
        u32 indexCount;
    };

    struct Mesh
    {
        Terrain::MapObjectFlags renderFlags;

        u32 baseIndexOffset;
        u32 baseVertexOffset;
        u32 baseMaterialOffset;
    };

    struct MapObjectToBeLoaded
    {
        const Terrain::Placement* placement = nullptr;
        const std::string* nmorName = nullptr;
        u32 nmorNameHash = 0;

        MeshRoot meshRoot;
        std::vector<Mesh> meshes;
    };

    struct DrawParameters
    {
        u32 indexCount;
        u32 instanceCount;
        u32 firstIndex;
        u32 vertexOffset;
        u32 firstInstance;
    };

    struct MaterialParameters
    {
        u16 materialID;
        u16 exteriorLit;
    };

    struct RenderBatchOffsets
    {
        u32 baseVertexOffset;
        u32 baseIndexOffset;
    };

    struct LoadedMapObject
    {
        std::vector<u32> drawParameterIDs;
        std::vector<u16> materialParameterIDs;

        std::vector<u16> instanceIDs;
        std::vector<u32> instanceMaterialParameterIDs;

        std::vector<u32> vertexColors[2];

        u32 vertexColorTextureIDs[2] = { 0, 0 };
        u32 instanceCount;

        u32 baseVertexOffset = 0;
        u32 baseMaterialOffset = 0;
        u32 baseCullingDataOffset = 0;

        // Renderbatches
        std::vector<Terrain::RenderBatch> renderBatches;
        std::vector<RenderBatchOffsets> renderBatchOffsets;

        // Culling data
        std::vector<Terrain::CullingData> cullingData;
    };

public:
    MapObjectRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer);

    void Update(f32 deltaTime);

    void AddMapObjectPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

    void RegisterMapObjectToBeLoaded(const std::string& mapObjectName, const Terrain::Placement& mapObjectPlacement);
    void RegisterMapObjectsToBeLoaded(const Terrain::Chunk& chunk, StringTable& stringTable);
    void ExecuteLoad();

    void Clear();

    const std::vector<Terrain::CullingData>& GetCullingData() { return _cullingData; }

private:
    void CreatePermanentResources();
    bool LoadMapObject(MapObjectToBeLoaded& mapObjectToBeLoaded, LoadedMapObject& mapObject);

    // Sub loaders
    bool LoadRoot(const std::filesystem::path nmorPath, MeshRoot& meshRoot, LoadedMapObject& mapObject);
    bool LoadMesh(const std::filesystem::path nmoPath, Mesh& mesh, LoadedMapObject& mapObject);

    bool LoadIndicesAndVertices(Bytebuffer& buffer, Mesh& mesh, LoadedMapObject& mapObject);

    bool LoadRenderBatches(Bytebuffer& buffer, Mesh& mesh, LoadedMapObject& mapObject);

    void AddInstance(LoadedMapObject& mapObject, const Terrain::Placement* placement);

    void CreateBuffers();

    struct Material
    {
        u16 textureIDs[3] = { 0,0,0 };
        f16 alphaTestVal = f16(-1.0f);//1.0f / 255.0f;//1.0f / 16.0f;//1.0f / 255.0f;
        u16 materialType = 0;
        u16 unlit = 0;
    };

    struct InstanceLookupData
    {
        u16 instanceID;
        u16 materialParamID;
        u16 cullingDataID;
        u16 vertexColorTextureID0 = 0;
        u16 vertexColorTextureID1 = 0;
        u16 padding1;
        u32 vertexOffset;
    };

    struct InstanceData
    {
        mat4x4 instanceMatrix;
    };

    struct MeshData
    {
        u32 materialID;
        u32 renderFlags;
        u32 vertexColorTextureID;
        u32 vertexUVIndex;
    };

    struct CullingConstants
    {
        vec4 frustumPlanes[6];
        vec3 cameraPos;
        u32 maxDrawCount;
    };

private:
    Renderer::Renderer* _renderer;
    DebugRenderer* _debugRenderer;

    Renderer::SamplerID _sampler;
    Renderer::DescriptorSet _cullingDescriptorSet;
    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _meshDescriptorSet;

    std::vector<LoadedMapObject> _loadedMapObjects;
    robin_hood::unordered_map<u32, u32> _nameHashToIndexMap;

    std::vector<DrawParameters> _drawParameters;
    std::vector<u16> _indices;
    std::vector<Terrain::MapObjectVertex> _vertices;
    std::vector<InstanceData> _instances;
    std::vector<InstanceLookupData> _instanceLookupData;
    std::vector<Material> _materials;
    std::vector<MaterialParameters> _materialParameters;
    std::vector<Terrain::CullingData> _cullingData;

    Renderer::Buffer<CullingConstants>* _cullingConstantBuffer;

    Renderer::BufferID _argumentBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _culledArgumentBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _drawCountBuffer = Renderer::BufferID::Invalid();

    Renderer::BufferID _vertexBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _indexBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _instanceBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _instanceLookupBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _materialBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _materialParametersBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _cullingDataBuffer = Renderer::BufferID::Invalid();

    Renderer::TextureArrayID _mapObjectTextures;

    std::vector<MapObjectToBeLoaded> _mapObjectsToBeLoaded;
};