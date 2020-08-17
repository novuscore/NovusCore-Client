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

namespace Terrain
{
    struct Map;

    constexpr u32 NUM_VERTICES_PER_CHUNK = Terrain::CELL_TOTAL_GRID_SIZE * Terrain::MAP_CELLS_PER_CHUNK;
    constexpr u32 NUM_INDICES_PER_CELL = 768;
}

namespace Renderer
{
    class RenderGraph;
    class Renderer;
    class DescriptorSet;
}

class Camera;
class DebugRenderer;
class MapObjectRenderer;

struct BoundingBox
{
    vec3 min;
    vec3 max;
};

class TerrainRenderer
{
public:
    TerrainRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer);
    ~TerrainRenderer();

    void Update(f32 deltaTime, const Camera& camera);

    void AddTerrainDepthPrepass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::DepthImageID depthTarget, u8 frameIndex);
    void AddTerrainPass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex, u8 debugMode, const Camera& camera);

    bool LoadMap(u32 mapInternalNameHash);
private:
    void CreatePermanentResources();

    void LoadChunk(Terrain::Map& map, u16 chunkPosX, u16 chunkPosY);
    void LoadChunksAround(Terrain::Map& map, ivec2 middleChunk, u16 drawDistance);
    void CPUCulling(const Camera& camera);

private:
    Renderer::Renderer* _renderer;

    struct CullingConstants
    {
        vec4 frustumPlanes[6];
    };

    Renderer::Buffer<CullingConstants>* _cullingConstantBuffer;

    Renderer::BufferID _argumentBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _instanceBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _culledInstanceBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _cellHeightRangeBuffer = Renderer::BufferID::Invalid();

    Renderer::BufferID _chunkBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _cellBuffer = Renderer::BufferID::Invalid();
    Renderer::BufferID _vertexBuffer = Renderer::BufferID::Invalid();

    Renderer::BufferID _cellIndexBuffer = Renderer::BufferID::Invalid();
    
    Renderer::TextureArrayID _terrainColorTextureArray = Renderer::TextureArrayID::Invalid();
    Renderer::TextureArrayID _terrainAlphaTextureArray = Renderer::TextureArrayID::Invalid();

    Renderer::SamplerID _alphaSampler;
    Renderer::SamplerID _colorSampler;

    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _drawDescriptorSet;

    Renderer::DescriptorSet _cullingPassDescriptorSet;

    std::vector<u16> _loadedChunks;
    std::vector<BoundingBox> _cellBoundingBoxes;

    std::vector<u32> _culledInstances;
    
    // Subrenderers
    MapObjectRenderer* _mapObjectRenderer = nullptr;
    DebugRenderer* _debugRenderer = nullptr;
};