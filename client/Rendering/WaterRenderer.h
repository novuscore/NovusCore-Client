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

class CameraFreeLook;
class DebugRenderer;

class WaterRenderer
{
public:
    WaterRenderer(Renderer::Renderer* renderer);
    ~WaterRenderer();

    void Update(f32 deltaTime);
    void LoadWater(const std::vector<u16>& chunkIDs);
    void Clear();

    void AddWaterPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

private:
    void CreatePermanentResources();

    bool RegisterChunksToBeLoaded(const std::vector<u16>& chunkIDs);
    void ExecuteLoad();
    
    struct Instance
    {
        mat4x4 instanceMatrix;
    };

    static const u32 MAX_INSTANCES = 100000;
    struct LoadedWater
    {
        std::string debugName = "";

        u32 numInstances = 0;
        Renderer::BufferID instanceBuffer; // One per instance
    };

    struct ChunkToBeLoaded
    {
        mat4x4 instanceMatrix;
        f16 vertexHeightValues[4] = { static_cast<f16>(0), static_cast<f16>(0), static_cast<f16>(0), static_cast<f16>(0) };
    };

    Renderer::Renderer* _renderer;

    Renderer::SamplerID _sampler;
    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _meshDescriptorSet;

    Renderer::BufferID _vertexBuffer;
    Renderer::BufferID _indexBuffer;

    u32 _textureIndex = 0;
    Renderer::TextureArrayID _waterTextures;

    std::vector<LoadedWater> _loadedWater;
    std::vector<ChunkToBeLoaded> _chunksToBeLoaded;
};