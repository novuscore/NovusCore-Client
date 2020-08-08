#include "Renderer.h"

namespace Renderer
{
    Renderer::~Renderer()
    {
    }

    RenderGraph Renderer::CreateRenderGraph(RenderGraphDesc& desc)
    {
        RenderGraph renderGraph(desc.allocator, this);
        renderGraph.Init(desc);

        return renderGraph;
    }

    DescriptorSetBackend* Renderer::CreateDescriptorSetBackend()
    {
        return nullptr;
    }
}