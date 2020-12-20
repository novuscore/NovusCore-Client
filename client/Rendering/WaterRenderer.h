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
    
    struct Constants
    {
        f32 currentTime = 0;
    };

    struct DrawCall
    {
        u32 indexCount;
        u32 instanceCount;
        u32 firstIndex;
        u32 vertexOffset;
        u32 firstInstance;
    };

    struct DrawCallData
    {
        u32 textureStartIndex;
        u32 textureCount;
    };

    Renderer::Renderer* _renderer;

    Renderer::SamplerID _sampler;
    Renderer::DescriptorSet _passDescriptorSet;

    Renderer::BufferID _drawCallsBuffer;
    Renderer::BufferID _drawCallDatasBuffer;
    Renderer::BufferID _vertexBuffer;
    Renderer::BufferID _indexBuffer;

    Renderer::TextureArrayID _waterTextures;

    std::vector<DrawCall> _drawCalls;
    std::vector<DrawCallData> _drawCallDatas;

    std::vector<vec4> _vertices;
    std::vector<u16> _indices;

    Constants _constants;
};