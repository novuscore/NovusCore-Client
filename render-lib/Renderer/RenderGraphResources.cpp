#include "RenderGraphResources.h"
#include "Descriptors/GraphicsPipelineDesc.h"

namespace Renderer
{
	RenderGraphResources::RenderGraphResources(Memory::Allocator* allocator)
		: _trackedImages(allocator, 32)
		, _trackedTextures(allocator, 32)
		, _trackedDepthImages(allocator, 32)
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

    ImageID RenderGraphResources::GetImage(RenderPassResource resource)
    {
        using type = type_safe::underlying_type<RenderPassResource>;
        return _trackedImages[static_cast<type>(resource)];
    }

    ImageID RenderGraphResources::GetImage(RenderPassMutableResource resource)
    {
        using type = type_safe::underlying_type<RenderPassMutableResource>;
        return _trackedImages[static_cast<type>(resource)];
    }

    DepthImageID RenderGraphResources::GetDepthImage(RenderPassResource resource)
    {
        using type = type_safe::underlying_type<RenderPassResource>;
        return _trackedDepthImages[static_cast<type>(resource)];
    }

    DepthImageID RenderGraphResources::GetDepthImage(RenderPassMutableResource resource)
    {
        using type = type_safe::underlying_type<RenderPassMutableResource>;
        return _trackedDepthImages[static_cast<type>(resource)];
    }

    RenderPassResource RenderGraphResources::GetResource(ImageID id)
    {
        using _type = type_safe::underlying_type<ImageID>;

        _type i = 0;
        for (ImageID& trackedID : _trackedImages)
        {
            if (trackedID == id)
            {
                return RenderPassResource(i);
            }

            i++;
        }

        _trackedImages.Insert(id);
        return RenderPassResource(i);
    }

    RenderPassResource RenderGraphResources::GetResource(TextureID id)
    {
        using _type = type_safe::underlying_type<TextureID>;

        _type i = 0;
        for (TextureID& trackedID : _trackedTextures)
        {
            if (trackedID == id)
            {
                return RenderPassResource(i);
            }

            i++;
        }

        _trackedTextures.Insert(id);
        return RenderPassResource(i);
    }

    RenderPassResource RenderGraphResources::GetResource(DepthImageID id)
    {
        using _type = type_safe::underlying_type<DepthImageID>;

        _type i = 0;
        for (DepthImageID& trackedID : _trackedDepthImages)
        {
            if (trackedID == id)
            {
                return RenderPassResource(i);
            }

            i++;
        }

        _trackedDepthImages.Insert(id);
        return RenderPassResource(i);
    }

    RenderPassMutableResource RenderGraphResources::GetMutableResource(ImageID id)
    {
        using _type = type_safe::underlying_type<ImageID>;

        _type i = 0;
        for (ImageID& trackedID : _trackedImages)
        {
            if (trackedID == id)
            {
                return RenderPassMutableResource(i);
            }

            i++;
        }

        _trackedImages.Insert(id);
        return RenderPassMutableResource(i);
    }

    RenderPassMutableResource RenderGraphResources::GetMutableResource(DepthImageID id)
    {
        using _type = type_safe::underlying_type<DepthImageID>;

        _type i = 0;
        for (DepthImageID& trackedID : _trackedDepthImages)
        {
            if (trackedID == id)
            {
                return RenderPassMutableResource(i);
            }

            i++;
        }

        _trackedDepthImages.Insert(id);
        return RenderPassMutableResource(i);
    }
}