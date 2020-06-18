#include "RenderGraph.h"
#include "RenderGraphBuilder.h"

#include "Renderer.h"

namespace Renderer
{
    bool RenderGraph::Init(RenderGraphDesc& desc)
    {
        _desc = desc;
        assert(desc.allocator != nullptr); // You need to set an allocator

        _renderGraphBuilder = Memory::Allocator::New<RenderGraphBuilder>(desc.allocator, desc.allocator, _renderer);

        return true;
    }

    RenderGraph::~RenderGraph()
    {
        for (IRenderPass* pass : _passes)
        {
            pass->DeInit();
        }
    }

    /*void RenderGraph::AddPass(RenderPass& pass)
    {
        _passes.push_back(pass);
    }*/

    void RenderGraph::Setup()
    {
        for (IRenderPass* pass : _passes)
        {
            if (pass->Setup(_renderGraphBuilder))
            {
                _executingPasses.Insert(pass);
            }
        }
    }

    void RenderGraph::Execute()
    {
        // TODO: Parallel_for this
        CommandList commandList(_renderer, _desc.allocator);
        commandList.PushMarker("RenderGraph", Color(0.0f, 0.0f, 0.4f));
        for (IRenderPass* pass : _executingPasses)
        {
            pass->Execute(commandList);
        }
        commandList.PopMarker();
        commandList.Execute();
    }

    void RenderGraph::InitializePipelineDesc(GraphicsPipelineDesc& desc) const
    {
        desc.ResourceToImageID = [&](RenderPassResource resource) 
        {
            return _renderGraphBuilder->GetImage(resource);
        };
        desc.ResourceToDepthImageID = [&](RenderPassResource resource) 
        {
            return _renderGraphBuilder->GetDepthImage(resource);
        };
        desc.MutableResourceToImageID = [&](RenderPassMutableResource resource) 
        {
            return _renderGraphBuilder->GetImage(resource);
        };
        desc.MutableResourceToDepthImageID = [&](RenderPassMutableResource resource) 
        {
            return _renderGraphBuilder->GetDepthImage(resource);
        };
    }
}