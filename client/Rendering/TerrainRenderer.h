#pragma once
#include <NovusTypes.h>

#include <array>

#include <Utils/StringUtils.h>
#include <Math/Geometry.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/TextureArrayDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Descriptors/BufferDesc.h>
#include <Renderer/Buffer.h>
#include <Renderer/DescriptorSet.h>

#include "../Gameplay/Map/Chunk.h"
#include "ViewConstantBuffer.h"

namespace Terrain
{
    struct Map;

    constexpr u32 NUM_VERTICES_PER_CHUNK = Terrain::MAP_CELL_TOTAL_GRID_SIZE * Terrain::MAP_CELLS_PER_CHUNK;
    constexpr u32 NUM_INDICES_PER_CELL = 768;
    constexpr u32 NUM_TRIANGLES_PER_CELL = NUM_INDICES_PER_CELL / 3;
}

namespace Renderer
{
    class RenderGraph;
    class Renderer;
    class DescriptorSet;
}

namespace NDBC
{
    struct Map;
}

class Camera;
class DebugRenderer;
class MapObjectRenderer;
class CModelRenderer;
class WaterRenderer;

class TerrainRenderer
{
    struct ChunkToBeLoaded
    {
        Terrain::Map* map = nullptr;
        Terrain::Chunk* chunk = nullptr;
        u16 chunkPosX;
        u16 chunkPosY;
        u16 chunkID;
    };

    struct CullingConstants
    {
        vec4 frustumPlanes[6];
        mat4x4 viewmat;
        u32 occlusionEnabled;
    };

    struct CellInstance
    {
        u32 packedChunkCellID;
        u32 instanceID;
    };

#pragma pack(push, 1)
    struct TerrainVertex
    {
        u8 normal[3];
        u8 color[3];
        f16 height;
    };
#pragma pack(pop)

public:
    TerrainRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer, CModelRenderer* complexModelRenderer);
    ~TerrainRenderer();

    void Update(f32 deltaTime);

    void AddTerrainPass(Renderer::RenderGraph* renderGraph, const Renderer::DescriptorSet* globalDescriptorSet, const Renderer::DescriptorSet* debugDescriptorSet, Renderer::ImageID colorTarget, Renderer::ImageID objectTarget, Renderer::DepthImageID depthTarget, Renderer::ImageID depthPyramid, u8 frameIndex);

    bool LoadMap(const NDBC::Map* map);

    const std::vector<Geometry::AABoundingBox>& GetBoundingBoxes() { return _cellBoundingBoxes; }
    MapObjectRenderer* GetMapObjectRenderer() { return _mapObjectRenderer; }

    // Drawcall stats
    u32 GetNumDrawCalls() { return Terrain::MAP_CELLS_PER_CHUNK * static_cast<u32>(_loadedChunks.size()); }
    u32 GetNumSurvivingDrawCalls() { return _numSurvivingDrawCalls; }

    // Triangle stats
    u32 GetNumTriangles() { return Terrain::MAP_CELLS_PER_CHUNK * static_cast<u32>(_loadedChunks.size()) * Terrain::NUM_TRIANGLES_PER_CELL; }
    u32 GetNumSurvivingTriangles() { return _numSurvivingDrawCalls * Terrain::NUM_TRIANGLES_PER_CELL; }
private:
    void CreatePermanentResources();

    void RegisterChunksToBeLoaded(Terrain::Map& map, ivec2 middleChunk, u16 drawDistance);
    void RegisterChunkToBeLoaded(Terrain::Map& map, u16 chunkPosX, u16 chunkPosY);
    void ExecuteLoad();

    void LoadChunk(const ChunkToBeLoaded& chunkToBeLoaded);
    //void LoadChunksAround(Terrain::Map& map, ivec2 middleChunk, u16 drawDistance);
    void CPUCulling(const Camera* camera);

    void DebugRenderCellTriangles(const Camera* camera);
private:
    Renderer::Renderer* _renderer; 
    
    CullingConstants _cullingConstants;

    Renderer::BufferID _instanceBuffer;
    Renderer::BufferID _culledInstanceBuffer;
    Renderer::BufferID _cellHeightRangeBuffer;
    Renderer::BufferID _argumentBuffer;
    Renderer::BufferID _drawCountReadBackBuffer;

    Renderer::BufferID _chunkBuffer;
    Renderer::BufferID _cellBuffer;

    Renderer::BufferID _vertexBuffer;

    Renderer::BufferID _cellIndexBuffer;
    
    Renderer::TextureArrayID _terrainColorTextureArray;
    Renderer::TextureArrayID _terrainAlphaTextureArray;

    Renderer::SamplerID _alphaSampler;
    Renderer::SamplerID _colorSampler;

    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _drawDescriptorSet;

    Renderer::DescriptorSet _cullingPassDescriptorSet;

    std::vector<u16> _loadedChunks;
    std::vector<Geometry::AABoundingBox> _cellBoundingBoxes;

    std::vector<CellInstance> _culledInstances;

    std::vector<ChunkToBeLoaded> _chunksToBeLoaded;

    u32 _numSurvivingDrawCalls;
    
    // Subrenderers
    MapObjectRenderer* _mapObjectRenderer = nullptr;
    CModelRenderer* _complexModelRenderer = nullptr;
    WaterRenderer* _waterRenderer = nullptr;
    DebugRenderer* _debugRenderer = nullptr;
};