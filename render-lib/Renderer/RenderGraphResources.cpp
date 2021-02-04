#include "RenderGraphResources.h"

#include "Descriptors/GraphicsPipelineDesc.h"
#include <Containers/DynamicArray.h>

namespace Renderer
{
    struct RenderGraphResourcesData : IRenderGraphResourcesData
    {
        RenderGraphResourcesData(Memory::Allocator* allocator)
            : trackedImages(allocator, 32)
            , trackedTextures(allocator, 32)
            , trackedDepthImages(allocator, 32)
        {

        }

        DynamicArray<ImageID> trackedImages;
        DynamicArray<TextureID> trackedTextures;
        DynamicArray<DepthImageID> trackedDepthImages;
    };

	RenderGraphResources::RenderGraphResources(Memory::Allocator* allocator)
		: _allocator(allocator)
        , _data(Memory::Allocator::New<RenderGraphResourcesData>(allocator, allocator))
    {
	}

    void RenderGraphResources::InitializePipelineDesc(GraphicsPipelineDesc& desc)
    {
        desc.ResourceToImageID = [&](RenderPassResource resource)
        {
            return GetImage(resource);
        };
        desc.ResourceToDepthImageID = [&](RenderPassResource resource)
        {
            return GetDepthImage(resource);
        };
        desc.MutableResourceToImageID = [&](RenderPassMutableResource resource)
        {
            return GetImage(resource);
        };
        desc.MutableResourceToDepthImageID = [&](RenderPassMutableResource resource)
        {
            return GetDepthImage(resource);
        };
    }

    void RenderGraphResources::InitializePipelineDesc(ComputePipelineDesc& desc)
    {
    }

    ImageID RenderGraphResources::GetImage(RenderPassResource resource)
    {
        RenderGraphResourcesData* data = static_cast<RenderGraphResourcesData*>(_data);
        return data->trackedImages[static_cast<RenderPassResource::type > (resource)];
    }

    ImageID RenderGraphResources::GetImage(RenderPassMutableResource resource)
    {
        RenderGraphResourcesData* data = static_cast<RenderGraphResourcesData*>(_data);
        return data->trackedImages[static_cast<RenderPassMutableResource::type>(resource)];
    }

    DepthImageID RenderGraphResources::GetDepthImage(RenderPassResource resource)
    {
        RenderGraphResourcesData* data = static_cast<RenderGraphResourcesData*>(_data);
        return data->trackedDepthImages[static_cast<RenderPassResource::type>(resource)];
    }

    DepthImageID RenderGraphResources::GetDepthImage(RenderPassMutableResource resource)
    {
        RenderGraphResourcesData* data = static_cast<RenderGraphResourcesData*>(_data);
        return data->trackedDepthImages[static_cast<RenderPassMutableResource::type>(resource)];
    }

    RenderPassResource RenderGraphResources::GetResource(ImageID id)
    {
        RenderGraphResourcesData* data = static_cast<RenderGraphResourcesData*>(_data);

        ImageID::type i = 0;
        for (ImageID& trackedID : data->trackedImages)
        {
            if (trackedID == id)
            {
                return RenderPassResource(i);
            }

            i++;
        }

        data->trackedImages.Insert(id);
        return RenderPassResource(i);
    }

    RenderPassResource RenderGraphResources::GetResource(TextureID id)
    {
        RenderGraphResourcesData* data = static_cast<RenderGraphResourcesData*>(_data);

        TextureID::type i = 0;
        for (TextureID& trackedID : data->trackedTextures)
        {
            if (trackedID == id)
            {
                return RenderPassResource(i);
            }

            i++;
        }

        data->trackedTextures.Insert(id);
        return RenderPassResource(i);
    }

    RenderPassResource RenderGraphResources::GetResource(DepthImageID id)
    {
        RenderGraphResourcesData* data = static_cast<RenderGraphResourcesData*>(_data);

        DepthImageID::type i = 0;
        for (DepthImageID& trackedID : data->trackedDepthImages)
        {
            if (trackedID == id)
            {
                return RenderPassResource(i);
            }

            i++;
        }

        data->trackedDepthImages.Insert(id);
        return RenderPassResource(i);
    }

    RenderPassMutableResource RenderGraphResources::GetMutableResource(ImageID id)
    {
        RenderGraphResourcesData* data = static_cast<RenderGraphResourcesData*>(_data);

        ImageID::type i = 0;
        for (ImageID& trackedID : data->trackedImages)
        {
            if (trackedID == id)
            {
                return RenderPassMutableResource(i);
            }

            i++;
        }

        data->trackedImages.Insert(id);
        return RenderPassMutableResource(i);
    }

    RenderPassMutableResource RenderGraphResources::GetMutableResource(DepthImageID id)
    {
        RenderGraphResourcesData* data = static_cast<RenderGraphResourcesData*>(_data);

        DepthImageID::type i = 0;
        for (DepthImageID& trackedID : data->trackedDepthImages)
        {
            if (trackedID == id)
            {
                return RenderPassMutableResource(i);
            }

            i++;
        }

        data->trackedDepthImages.Insert(id);
        return RenderPassMutableResource(i);
    }
}