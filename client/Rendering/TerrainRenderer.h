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
#include <Renderer/ConstantBuffer.h>

#include "../Gameplay/Map/Chunk.h"
#include "Renderer/InstanceData.h"
#include "ViewConstantBuffer.h"

namespace Terrain
{
    struct Map;
}

namespace Renderer
{
    class RenderGraph;
    class Renderer;
}

class TerrainRenderer
{
public:
    TerrainRenderer(Renderer::Renderer* renderer);

    void Update(f32 deltaTime);
    void AddTerrainPass(Renderer::RenderGraph* renderGraph, Renderer::ConstantBuffer<ViewConstantBuffer>& viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

private:
    void CreatePermanentResources();
    void LoadChunk(Terrain::Map& map, u16 chunkPosX, u16 chunkPosY);
    void LoadChunksAround(Terrain::Map& map, ivec2 middleChunk, u16 radius);

private:
    Renderer::Renderer* _renderer;

    std::vector<Renderer::ModelID> _chunkModels;
    std::vector<Renderer::InstanceData> _chunkModelInstances;

    Renderer::TextureArrayID _terrainTextureArray;
    Renderer::TextureID _terrainTexture;
    Renderer::SamplerID _linearSampler;
};