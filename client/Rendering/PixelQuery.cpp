#include "PixelQuery.h"
#include "../Utils/ServiceLocator.h"

#include <tracy/Tracy.hpp>

PixelQuery::PixelQuery(Renderer::Renderer* renderer) : _renderer(renderer)
{
    CreatePermanentResources();
}

void PixelQuery::CreatePermanentResources()
{
    {
        Renderer::BufferDesc desc;
        desc.name = "PixelQueryResultBuffer";
        desc.size = sizeof(PixelQuery::PixelData) * MaxQueryRequestPerFrame;
        desc.usage = Renderer::BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER | Renderer::BUFFER_USAGE_STORAGE_BUFFER;
        desc.cpuAccess = Renderer::BufferCPUAccess::ReadOnly;
        _pixelResultBuffer = _renderer->CreateBuffer(desc);
    }

    _generator.seed(_randomDevice());
}

void PixelQuery::Update(f32 deltaTime)
{
}

void PixelQuery::AddPixelQueryPass(Renderer::RenderGraph* renderGraph, Renderer::ImageID colorTarget, Renderer::ImageID objectTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
    u32 numResultsToProcess = _numRequestsLastFrame[_frameIndex];
    if (numResultsToProcess > 0)
    {
        ZoneScopedN("Update::Process");
        std::vector<PixelData> results(numResultsToProcess);

        void* dst = _renderer->MapBuffer(_pixelResultBuffer);
        memcpy(results.data(), dst, sizeof(PixelData) * numResultsToProcess);
        _renderer->UnmapBuffer(_pixelResultBuffer);

        auto& tokenItr = _requestTokens[_frameIndex].begin();
        for (u32 i = 0; i < numResultsToProcess; i++)
        {
            const PixelData& pixelData = results[i];

            // Store Result & Remove Token from _requestTokens
            _results[*tokenItr] = pixelData;
            tokenItr = _requestTokens[_frameIndex].erase(tokenItr);
        }

        _numRequestsLastFrame[_frameIndex] = 0;
    }

    // Pixel Query Pass
    {
        struct PixelQueryPassData
        {
            Renderer::RenderPassMutableResource mainColor;
            Renderer::RenderPassMutableResource mainObject;
            Renderer::RenderPassMutableResource mainDepth;
        };

        renderGraph->AddPass<PixelQueryPassData>("Query Pass",
            [=](PixelQueryPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
            {
                data.mainColor = builder.Write(colorTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
                data.mainObject = builder.Write(objectTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
                data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);

                return true; // Return true from setup to enable this pass, return false to disable it
            },
            [=](PixelQueryPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
            {
                u32 numRequests = static_cast<u32>(_requests[_frameIndex].size());
                if (numRequests > 0)
                {
                    GPU_SCOPED_PROFILER_ZONE(commandList, QueryPass);

                    std::string frameIndexStr = "FrameIndex: " + std::to_string(_frameIndex);
                    TracyMessage(frameIndexStr.c_str(), frameIndexStr.length());

                    {
                        ZoneScopedN("PixelQuery::ImageBarrier");
                        commandList.ImageBarrier(objectTarget);
                    }
                    commandList.PushMarker("Pixel Queries " + std::to_string(numRequests), Color::White);
                    Renderer::ComputePipelineDesc queryPipelineDesc;
                    resources.InitializePipelineDesc(queryPipelineDesc);

                    Renderer::ComputeShaderDesc shaderDesc;
                    shaderDesc.path = "objectQuery.cs.hlsl";
                    queryPipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);

                    // Do culling
                    Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(queryPipelineDesc);
                    commandList.BeginPipeline(pipeline);

                    // Set Number of Requests we processed
                    _numRequestsLastFrame[_frameIndex] = numRequests;

                    // Copy Request Data to PixelDataBuffer
                    {
                        ZoneScopedN("PixelQuery::PushConstant");
                        QueryRequestConstant* queryRequests = resources.FrameNew<QueryRequestConstant>();

                        queryRequests->numRequests = numRequests;
                        std::memcpy(&queryRequests->pixelCoords[0], _requests[_frameIndex].data(), sizeof(QueryRequest) * numRequests);
                        commandList.PushConstant(queryRequests, 0, sizeof(QueryRequestConstant));
                    }

                    _queryDescriptorSet.Bind("_texture", objectTarget);
                    _queryDescriptorSet.Bind("_result", _pixelResultBuffer);
                    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_queryDescriptorSet, _frameIndex);

                    commandList.Dispatch(1, 1, 1);

                    commandList.EndPipeline(pipeline);
                    commandList.PopMarker();

                    _requests[_frameIndex].clear();
                }
                _frameIndex = !_frameIndex;
            });
    }
}

u32 PixelQuery::PerformQuery(uvec2 pixelCoords)
{
    ZoneScoped;

    std::string frameIndexStr = "FrameIndex: " + std::to_string(_frameIndex);
    TracyMessage(frameIndexStr.c_str(), frameIndexStr.length());

    u32 token = 0;

    _requestMutex.lock();
    {
        assert(_requests[_frameIndex].size() <= MaxQueryRequestPerFrame);

        QueryRequest& queryRequest = _requests[_frameIndex].emplace_back();
        queryRequest.pixelCoords = pixelCoords;

        std::uniform_int_distribution<u32> dist{ 1, 1000000000 };

        while (true)
        {
            // Insert on a std::set will return an iterator pointing to an existing element or a newly inserted one
            // We must also check the existing _results to ensure we we didn't create a duplicate token from an existing token

            token = dist(_generator);
            auto& result = _requestTokens[_frameIndex].insert(token);

            bool didInsertNewToken = result.second;
            if (didInsertNewToken)
            {
                bool isDuplicate = false;
                auto& setItr = _requestTokens[!_frameIndex].find(token);
                auto& resultItr = _results.find(token);

                isDuplicate |= setItr != _requestTokens[!_frameIndex].end();
                isDuplicate |= resultItr != _results.end();

                if (isDuplicate)
                    _requestTokens[_frameIndex].erase(result.first);
                else
                    break;
            }
        }
    }
    _requestMutex.unlock();

    return token;
}

bool PixelQuery::GetQueryResult(u32 token, PixelQuery::PixelData& pixelData)
{
    ZoneScoped;

    bool didFindResult = false;
    _resultMutex.lock();
    {
        didFindResult = _results.find(token) != _results.end();
        if (didFindResult)
        {
            pixelData = _results[token];
        }

        _resultMutex.unlock();
    }

    return didFindResult;
}

bool PixelQuery::FreeToken(u32 token)
{
    ZoneScoped;

    std::string frameIndexStr = "FrameIndex: " + std::to_string(_frameIndex);
    TracyMessage(frameIndexStr.c_str(), frameIndexStr.length());

    // Check if token is in _requestTokens
    {
        _requestMutex.lock();

        for (u32 i = 0; i < 2; i++)
        {
            std::set<u32>& requestTokens = _requestTokens[i];

            auto& itr = requestTokens.find(token);
            if (itr != requestTokens.end())
            {
                requestTokens.erase(itr);
                _requestMutex.unlock();

                return true;
            }
        }

        _requestMutex.unlock();
    }

    // Check if token is in _results
    {
        _resultMutex.lock();

        auto& itr = _results.find(token);
        if (itr != _results.end())
        {
            itr->second.type = 0;
            itr->second.value = 0;
            _results.erase(itr);

            _resultMutex.unlock();
            return true;
        }

        _resultMutex.unlock();
    }

    return false;
}
