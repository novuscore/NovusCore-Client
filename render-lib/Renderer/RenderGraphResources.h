#pragma once
#include <NovusTypes.h>
#include <vector>
#include <functional>

#include <Containers/DynamicArray.h>
#include <Memory/Allocator.h>

#include "RenderStates.h"
#include "RenderPassResources.h"

#include "Descriptors/TextureDesc.h"
#include "Descriptors/ImageDesc.h"
#include "Descriptors/DepthImageDesc.h"

namespace Renderer
{
    struct GraphicsPipelineDesc;
    struct ComputePipelineDesc;

    class RenderGraphResources
    {
    public:
        void InitializePipelineDesc(GraphicsPipelineDesc& desc);
        void InitializePipelineDesc(ComputePipelineDesc& desc);

        template<typename T, typename... Args>
        T* FrameNew(Args... args)
        {
            return Memory::Allocator::New<T>(_allocator, args...);
        }

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
        Memory::Allocator* _allocator = nullptr;

        DynamicArray<ImageID> _trackedImages;
        DynamicArray<TextureID> _trackedTextures;
        DynamicArray<DepthImageID> _trackedDepthImages;

        friend class RenderGraphBuilder;
    };
}