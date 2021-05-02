#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>
#include <filesystem>
#include <Utils/ByteBuffer.h>

#include <Renderer/Buffer.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
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
    struct PlacementDetails;
}

class StringTable;
class DebugRenderer;
struct RenderResources;

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
        u32 baseVertexColor1Offset;
        u32 baseVertexColor2Offset;
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
        u32 baseVertexColor1Offset;
        u32 baseVertexColor2Offset;
    };

public:
    struct LoadedMapObject
    {
        u32 objectID;
        std::string debugName = "";

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

    struct InstanceLookupData
    {
        u16 instanceID;
        u16 materialParamID;
        u16 cullingDataID;
        u16 vertexColorTextureID0 = 0;
        u16 vertexColorTextureID1 = 0;
        u16 padding1;
        u32 vertexOffset;
        u32 vertexColor1Offset;
        u32 vertexColor2Offset;
        u32 loadedObjectID;
    };

    struct InstanceData
    {
        mat4x4 instanceMatrix;
    };

public:
    MapObjectRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer);

    void Update(f32 deltaTime);

    void AddMapObjectDepthPrepass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex);
    void AddMapObjectPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex);

    void RegisterMapObjectToBeLoaded(const std::string& mapObjectName, const Terrain::Placement& mapObjectPlacement);
    void RegisterMapObjectsToBeLoaded(u16 chunkID, const Terrain::Chunk& chunk, StringTable& stringTable);
    void ExecuteLoad();

    void Clear();

    const std::vector<LoadedMapObject>& GetLoadedMapObjects() { return _loadedMapObjects; }
    const std::vector<InstanceData>& GetInstances() { return _instances; }
    const std::vector<Terrain::PlacementDetails>& GetPlacementDetails() { return _mapObjectPlacementDetails; }
    const std::vector<InstanceLookupData>& GetInstanceLookupData() { return _instanceLookupData; }

    u32 GetChunkPlacementDetailsOffset(u16 chunkID) { return _mapChunkToPlacementOffset[chunkID]; }
    u32 GetNumLoadedMapObjects() { return static_cast<u32>(_loadedMapObjects.size()); }
    u32 GetNumMapObjectPlacements() { return static_cast<u32>(_instances.size()); }

    // Drawcall stats
    u32 GetNumDrawCalls() { return static_cast<u32>(_drawParameters.size()); }
    u32 GetNumSurvivingDrawCalls() { return _numSurvivingDrawCalls; }

    // Triangle stats
    u32 GetNumTriangles() { return _numTriangles; }
    u32 GetNumSurvivingTriangles() { return _numSurvivingTriangles; }

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
        u32 occlusionEnabled;
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

    Renderer::BufferID _argumentBuffer;
    Renderer::BufferID _culledArgumentBuffer;
    Renderer::BufferID _drawCountBuffer;
    Renderer::BufferID _drawCountReadBackBuffer;
    Renderer::BufferID _triangleCountBuffer;
    Renderer::BufferID _triangleCountReadBackBuffer;

    Renderer::BufferID _vertexBuffer;
    Renderer::BufferID _indexBuffer;
    Renderer::BufferID _instanceBuffer;
    Renderer::BufferID _instanceLookupBuffer;
    Renderer::BufferID _materialBuffer;
    Renderer::BufferID _materialParametersBuffer;
    Renderer::BufferID _cullingDataBuffer;

    Renderer::TextureArrayID _mapObjectTextures;

    robin_hood::unordered_map<u32, u8> _uniqueIdCounter;
    robin_hood::unordered_map<u16, u32> _mapChunkToPlacementOffset;
    std::vector<Terrain::PlacementDetails> _mapObjectPlacementDetails;

    u32 _numSurvivingDrawCalls;
    u32 _numTriangles;
    u32 _numSurvivingTriangles;

    std::vector<MapObjectToBeLoaded> _mapObjectsToBeLoaded;
};