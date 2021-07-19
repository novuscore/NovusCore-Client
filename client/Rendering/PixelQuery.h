#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>
#include <random>
#include <set>

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Descriptors/SemaphoreDesc.h>
#include <Renderer/DescriptorSet.h>
#include <Renderer/FrameResource.h>
#include <Renderer/Buffer.h>

struct RenderResources;

class PixelQuery
{
public:
    struct PixelData
    {
        u32 type = 0;
        u32 value = 0;
    };

public:
    PixelQuery(Renderer::Renderer* renderer);

    void Update(f32 deltaTime);
    void AddPixelQueryPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex);

    u32 PerformQuery(uvec2 pixelCoords);
    bool GetQueryResult(u32 token, PixelQuery::PixelData& pixelData);
    bool FreeToken(u32 token);

private:
    void CreatePermanentResources();

private:
    static const u32 MaxQueryRequestPerFrame = 15;
    struct QueryRequest
    {
        uvec2 pixelCoords;
    };

    struct QueryRequestConstant
    {
        uvec2 pixelCoords[MaxQueryRequestPerFrame];
        u32 numRequests;
    };

    u32 _frameIndex = 0;
    u32 _numRequestsLastFrame[2] = { 0, 0 };

    std::vector<QueryRequest> _requests[2];
    std::set<u32> _requestTokens[2];

    robin_hood::unordered_map<u32, PixelData> _results;

    std::mutex _requestMutex;
    std::mutex _resultMutex;

    std::random_device _randomDevice;
    std::mt19937 _generator;

private:
    Renderer::Renderer* _renderer;

    Renderer::DescriptorSet _queryDescriptorSet;
    Renderer::BufferID _pixelResultBuffer;
};