#include "ImageHandlerVK.h"
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include "RenderDeviceVK.h"
#include "FormatConverterVK.h"
#include "DebugMarkerUtilVK.h"

namespace Renderer
{
    namespace Backend
    {
        ImageHandlerVK::ImageHandlerVK()
        {

        }

        ImageHandlerVK::~ImageHandlerVK()
        {

        }

        ImageID ImageHandlerVK::CreateImage(RenderDeviceVK* device, const ImageDesc& desc)
        {
            size_t nextHandle = _images.size();

            // Make sure we haven't exceeded the limit of the ImageID type, if this hits you need to change type of ImageID to something bigger
            assert(nextHandle < ImageID::MaxValue());
            using type = type_safe::underlying_type<ImageID>;

            Image image;
            image.desc = desc;

            assert(desc.dimensions.x > 0); // Make sure the width is valid
            assert(desc.dimensions.y > 0); // Make sure the height is valid
            assert(desc.depth > 0); // Make sure the depth is valid
            assert(desc.format != IMAGE_FORMAT_UNKNOWN); // Make sure the format is valid

            // Create image
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.pNext = nullptr;
            imageInfo.flags = 0; // TODO: VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT and VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT? https://github.com/DiligentGraphics/DiligentCore/blob/1edcafe9bd41bdde86869d4e1c0212c78ce123b7/Graphics/GraphicsEngineVulkan/src/TextureVkImpl.cpp
            
            if (desc.depth == 1)
            {
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
            }
            else
            {
                NC_LOG_FATAL("Non-3d images is currently unsupported");
            }

            imageInfo.format = FormatConverterVK::ToVkFormat(desc.format);
            imageInfo.extent.width = desc.dimensions.x;
            imageInfo.extent.height = desc.dimensions.y;
            imageInfo.extent.depth = desc.depth;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = FormatConverterVK::ToVkSampleCount(desc.sampleCount);
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            // We will sample directly from the color attachment
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Transfers
            imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // RTV
            imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT; // SRV 
            imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT; // UAV

            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.queueFamilyIndexCount = 0;
            imageInfo.pQueueFamilyIndices = nullptr;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            if (vkCreateImage(device->_device, &imageInfo, nullptr, &image.image) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create image!");
            }

            DebugMarkerUtilVK::SetObjectName(device->_device, (u64)image.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, desc.debugName.c_str());

            // Bind memory
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device->_device, image.image, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = device->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(device->_device, &allocInfo, nullptr, &image.memory) != VK_SUCCESS) 
            {
                NC_LOG_FATAL("Failed to allocate image memory!");
            }

            vkBindImageMemory(device->_device, image.image, image.memory, 0);

            // Create Color View
            VkImageViewCreateInfo colorViewInfo = {};
            colorViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            colorViewInfo.image = image.image;
            if (desc.depth == 1)
            {
                colorViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            }
            else
            {
                NC_LOG_FATAL("Non-3d images is currently unsupported");
            }
            colorViewInfo.format = imageInfo.format;
            colorViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
            colorViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorViewInfo.subresourceRange.baseMipLevel = 0;
            colorViewInfo.subresourceRange.levelCount = 1;
            colorViewInfo.subresourceRange.baseArrayLayer = 0;
            colorViewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device->_device, &colorViewInfo, nullptr, &image.colorView) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create color image view!");
            }

            DebugMarkerUtilVK::SetObjectName(device->_device, (u64)image.colorView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, desc.debugName.c_str());
            
            // Transition image from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_GENERAL
            device->TransitionImageLayout(image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

            _images.push_back(image);

            return ImageID(static_cast<type>(nextHandle));
        }

        DepthImageID ImageHandlerVK::CreateDepthImage(RenderDeviceVK* device, const DepthImageDesc& desc)
        {
            size_t nextHandle = _depthImages.size();

            // Make sure we haven't exceeded the limit of the DepthImageID type, if this hits you need to change type of DepthImageID to something bigger
            assert(nextHandle < DepthImageID::MaxValue());
            using type = type_safe::underlying_type<DepthImageID>;

            DepthImage image;
            image.desc = desc;

            // Create image
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.pNext = nullptr;
            imageInfo.flags = 0; // TODO: VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT and VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT? https://github.com/DiligentGraphics/DiligentCore/blob/1edcafe9bd41bdde86869d4e1c0212c78ce123b7/Graphics/GraphicsEngineVulkan/src/TextureVkImpl.cpp
            imageInfo.imageType = VK_IMAGE_TYPE_2D;

            imageInfo.format = FormatConverterVK::ToVkFormat(desc.format);
            imageInfo.extent.width = desc.dimensions.x;
            imageInfo.extent.height = desc.dimensions.y;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = FormatConverterVK::ToVkSampleCount(desc.sampleCount);
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            // We will sample directly from the color attachment
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Transfers
            imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; // DSV
            imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT; // SRV 
            //imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT; // UAV TODO

            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.queueFamilyIndexCount = 0;
            imageInfo.pQueueFamilyIndices = nullptr;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            if (vkCreateImage(device->_device, &imageInfo, nullptr, &image.image) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create image!");
            }

            DebugMarkerUtilVK::SetObjectName(device->_device, (u64)image.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, desc.debugName.c_str());

            // Bind memory
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device->_device, image.image, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = device->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(device->_device, &allocInfo, nullptr, &image.memory) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to allocate image memory!");
            }

            vkBindImageMemory(device->_device, image.image, image.memory, 0);

            // Create Depth View
            VkImageViewCreateInfo depthViewInfo = {};
            depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            depthViewInfo.image = image.image;
            depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
           
            depthViewInfo.format = imageInfo.format;
            depthViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            depthViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            depthViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            depthViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthViewInfo.subresourceRange.baseMipLevel = 0;
            depthViewInfo.subresourceRange.levelCount = 1;
            depthViewInfo.subresourceRange.baseArrayLayer = 0;
            depthViewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device->_device, &depthViewInfo, nullptr, &image.depthView) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create depth image view!");
            }

            DebugMarkerUtilVK::SetObjectName(device->_device, (u64)image.depthView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, desc.debugName.c_str());

            // Transition image from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            device->TransitionImageLayout(image.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            _depthImages.push_back(image);

            return DepthImageID(static_cast<type>(nextHandle));
        }

        const ImageDesc& ImageHandlerVK::GetImageDesc(const ImageID id)
        {
            using type = type_safe::underlying_type<ImageID>;

            // Lets make sure this id exists
            assert(_images.size() > static_cast<type>(id));
            return _images[static_cast<type>(id)].desc;
        }

        const DepthImageDesc& ImageHandlerVK::GetDepthImageDesc(const DepthImageID id)
        {
            using type = type_safe::underlying_type<DepthImageID>;

            // Lets make sure this id exists
            assert(_depthImages.size() > static_cast<type>(id));
            return _depthImages[static_cast<type>(id)].desc;
        }

        VkImage ImageHandlerVK::GetImage(const ImageID id)
        {
            using type = type_safe::underlying_type<ImageID>;

            // Lets make sure this id exists
            assert(_images.size() > static_cast<type>(id));
            return _images[static_cast<type>(id)].image;
        }

        VkImageView ImageHandlerVK::GetColorView(const ImageID id)
        {
            using type = type_safe::underlying_type<ImageID>;

            // Lets make sure this id exists
            assert(_images.size() > static_cast<type>(id));
            return _images[static_cast<type>(id)].colorView;
        }

        VkImage ImageHandlerVK::GetImage(const DepthImageID id)
        {
            using type = type_safe::underlying_type<DepthImageID>;

            // Lets make sure this id exists
            assert(_depthImages.size() > static_cast<type>(id));
            return _depthImages[static_cast<type>(id)].image;
        }

        VkImageView ImageHandlerVK::GetDepthView(const DepthImageID id)
        {
            using type = type_safe::underlying_type<DepthImageID>;

            // Lets make sure this id exists
            assert(_depthImages.size() > static_cast<type>(id));
            return _depthImages[static_cast<type>(id)].depthView;
        }
    }
}