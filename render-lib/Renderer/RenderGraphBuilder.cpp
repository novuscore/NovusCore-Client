#include "RenderGraphBuilder.h"
#include "Renderer.h"
#include "RenderGraph.h"

namespace Renderer
{
    RenderGraphBuilder::RenderGraphBuilder(Memory::Allocator* allocator, Renderer* renderer)
        : _renderer(renderer)
        , _resources(allocator)
    {

    }

    void RenderGraphBuilder::Compile(CommandList* /*commandList*/)
    {

    }

    RenderGraphResources& RenderGraphBuilder::GetResources()
    {
        return _resources;
    }

    ImageID RenderGraphBuilder::Create(ImageDesc& /*desc*/)
    {
        return ImageID::Invalid();
    }

    DepthImageID RenderGraphBuilder::Create(DepthImageDesc& /*desc*/)
    {
        return DepthImageID::Invalid();
    }

    RenderPassResource RenderGraphBuilder::Read(ImageID id, ShaderStage /*shaderStage*/)
    {
        RenderPassResource resource = _resources.GetResource(id);

        return resource;
    }

    RenderPassResource RenderGraphBuilder::Read(TextureID id, ShaderStage /*shaderStage*/)
    {
        RenderPassResource resource = _resources.GetResource(id);

        return resource;
    }

    RenderPassResource RenderGraphBuilder::Read(DepthImageID id, ShaderStage /*shaderStage*/)
    {
        RenderPassResource resource = _resources.GetResource(id);

        return resource;
    }

    RenderPassMutableResource RenderGraphBuilder::Write(ImageID id, WriteMode /*writeMode*/, LoadMode /*loadMode*/)
    {
        RenderPassMutableResource resource = _resources.GetMutableResource(id);

        return resource;
    }

    RenderPassMutableResource RenderGraphBuilder::Write(DepthImageID id, WriteMode /*writeMode*/, LoadMode /*loadMode*/)
    {
        RenderPassMutableResource resource = _resources.GetMutableResource(id);

        return resource;
    }
}