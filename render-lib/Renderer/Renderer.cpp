#include "Renderer.h"
#include "RenderGraph.h"

#include <Memory/Allocator.h>

namespace Renderer
{
    Renderer::~Renderer()
    {
    }

    RenderGraph& Renderer::CreateRenderGraph(RenderGraphDesc& desc)
    {
        RenderGraph* renderGraph = Memory::Allocator::New<RenderGraph>(desc.allocator, desc.allocator, this);
        renderGraph->Init(desc);

        return *renderGraph;
    }
}