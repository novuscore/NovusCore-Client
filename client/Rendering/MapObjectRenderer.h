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
    struct MapObjectPlacement;
}

class StringTable;

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
    };

    struct MapObjectToBeLoaded
    {
        const Terrain::MapObjectPlacement* placement = nullptr;
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
        u32 materialID;
        u32 exteriorLit;
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
    };

public:
    MapObjectRenderer(Renderer::Renderer* renderer);

    void Update(f32 deltaTime);

    void AddMapObjectPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

    void RegisterMapObjectsToBeLoaded(const Terrain::Chunk& chunk, StringTable& stringTable);
    void ExecuteLoad();

    void Clear();

private:
    void CreatePermanentResources();
    bool LoadMapObject(MapObjectToBeLoaded& mapObjectToBeLoaded, LoadedMapObject& mapObject);

    // Sub loaders
    void LoadRoot(const std::filesystem::path nmorPath, MeshRoot& meshRoot, LoadedMapObject& mapObject);
    void LoadMesh(const std::filesystem::path nmoPath, Mesh& mesh, LoadedMapObject& mapObject);

    void LoadIndicesAndVertices(Bytebuffer& buffer, Mesh& mesh, LoadedMapObject& mapObject);

    void LoadRenderBatches(Bytebuffer& buffer, const Mesh& mesh, LoadedMapObject& mapObject);

    void AddInstance(LoadedMapObject& mapObject, const Terrain::MapObjectPlacement* placement);

    void CreateBuffers();

    struct Material
    {
        u32 textureIDs[3] = { 0,0,0 };
        f32 alphaTestVal = -1.0f;//1.0f / 255.0f;//1.0f / 16.0f;//1.0f / 255.0f;
        u32 materialType = 0;
        u32 unlit = 0;
    };

    struct InstanceLookupData
    {
        u16 instanceID;
        u16 materialParamID;
        u16 vertexColorTextureID0 = 0;
        u16 vertexColorTextureID1 = 0;
        u32 vertexOffset;
        u32 padding1;
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

private:
    Renderer::Renderer* _renderer;

    Renderer::SamplerID _sampler;
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

    Renderer::BufferID _indirectArgumentBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _vertexBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _indexBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _instanceBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _instanceLookupBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _materialBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _materialParametersBuffer = Renderer::BufferID::Invalid();

    Renderer::TextureArrayID _mapObjectTextures;

    std::vector<MapObjectToBeLoaded> _mapObjectsToBeLoaded;
};