#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>

#include <Renderer/Buffer.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>

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

    void AddMapObjectDepthPrepass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::DepthImageID depthTarget, u8 frameIndex);
    void AddMapObjectPass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

    void LoadMapObjects(Terrain::Chunk& chunk, StringTable& stringTable);

private:
    void CreatePermanentResources();
    u32 LoadMapObject(u32 nameID, StringTable& stringTable);

    struct LoadedMapObject
    {
        std::vector<Renderer::ModelID> modelIDs;
    };

private:
    Renderer::Renderer* _renderer;

    std::vector<LoadedMapObject> _loadedMapObjects;
    robin_hood::unordered_map<u32, u32> _nameHashToIndexMap;
};