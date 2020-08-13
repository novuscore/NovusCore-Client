#pragma once
#include <NovusTypes.h>
#include <vector>
#include <functional>

#include <Containers/DynamicArray.h>

#include "RenderStates.h"
#include "RenderPassResources.h"

#include "Descriptors/TextureDesc.h"
#include "Descriptors/ImageDesc.h"
#include "Descriptors/DepthImageDesc.h"

namespace Memory
{
    class Allocator;
}

namespace Renderer
{
    struct GraphicsPipelineDesc;
    struct ComputePipelineDesc;

    class RenderGraphResources
    {
    public:
        void InitializePipelineDesc(GraphicsPipelineDesc& desc);
        void InitializePipelineDesc(ComputePipelineDesc& desc);

    private:
        RenderGraphResources(Memory::Allocator* allocator);

        ImageID GetImage(RenderPassResource resource);
        ImageID GetImage(RenderPassMutableResource resource);
        DepthImageID GetDepthImage(RenderPassResource resource);
        DepthImageID GetDepthImage(RenderPassMutableResource resource);

        RenderPassResource GetResource(ImageID id);
        RenderPassResource GetResource(TextureID id);
        RenderPassResource GetResource(DepthImageID id);
        RenderPassMutableResource GetMutableResource(ImageID id);
        RenderPassMutableResource GetMutableResource(DepthImageID id);

    private:
        DynamicArray<ImageID> _trackedImages;
        DynamicArray<TextureID> _trackedTextures;
        DynamicArray<DepthImageID> _trackedDepthImages;

        friend class RenderGraphBuilder;
    };
}