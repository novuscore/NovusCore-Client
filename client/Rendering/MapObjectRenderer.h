#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>

#include <Renderer/Buffer.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/BufferDesc.h>

#include "ViewConstantBuffer.h"

namespace Renderer
{
    class RenderGraph;
    class Renderer;
    class DescriptorSet;
}

namespace Terrain
{
    struct Chunk;
}

class StringTable;

class MapObjectRenderer
{
public:
    MapObjectRenderer(Renderer::Renderer* renderer);

    void Update(f32 deltaTime);

    void AddMapObjectPass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

    void LoadMapObjects(const Terrain::Chunk& chunk, StringTable& stringTable);

private:
    void CreatePermanentResources();
    bool LoadMapObject(u32 nameID, StringTable& stringTable, u32& objectID);

    struct Material
    {
        u32 textureIDs[3];
        f32 alphaTestVal = -1.0f;//1.0f / 255.0f;//1.0f / 16.0f;//1.0f / 255.0f;
        u32 materialType = 0;
    };

    struct Instance
    {
        mat4x4 instanceMatrix;
    };

    struct Mesh
    {
        // Per submesh data
        std::vector<u32> numIndices;
        std::vector<Renderer::BufferID> indexBuffers; 
        std::vector<u32> materialIDs;

        // Per mesh data
        Renderer::BufferID vertexPositionsBuffer;
        Renderer::BufferID vertexNormalsBuffer;

        u32 numUVSets;
        Renderer::BufferID vertexUVsBuffer;
    };

    static const u32 MAX_INSTANCES = 256;
    struct LoadedMapObject
    {
        std::string debugName = "";

        std::vector<Mesh> meshes;

        Renderer::BufferID materialsBuffer;

        u32 numInstances;
        Renderer::BufferID instanceBuffer; // One per instance
    };

private:
    Renderer::Renderer* _renderer;

    Renderer::SamplerID _sampler;
    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _meshDescriptorSet;

    std::vector<LoadedMapObject> _loadedMapObjects;
    robin_hood::unordered_map<u32, u32> _nameHashToIndexMap;

    Renderer::TextureArrayID _mapObjectTextures;
};