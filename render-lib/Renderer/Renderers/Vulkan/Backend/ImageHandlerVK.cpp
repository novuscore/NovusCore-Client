#include "ImageHandlerVK.h"
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <vulkan/vulkan.h>
#include <vector>

#include "RenderDeviceVK.h"
#include "FormatConverterVK.h"
#include "DebugMarkerUtilVK.h"

namespace Renderer
{
    namespace Backend
    {
        u32 PreviousPow2(u32 v)
        {
            u32 r = 1;

            while (r * 2 < v)
                r *= 2;

            return r;
        }

        u32 GetImageMipLevels(u32 width, u32 height)
        {
            u32 result = 1;

            while (width > 1 || height > 1)
            {
                result++;
                width /= 2;
                height /= 2;
            }

            return result;
        }

        struct Image
        {
            ImageDesc desc;

            VmaAllocation allocation;
            VkImage image;
            VkImageView colorView;
            bool isSwapchain = false;
        };

        struct DepthImage
        {
            DepthImageDesc desc;

            VmaAllocation allocation;
            VkImage image;
            VkImageView depthView;
        };

        struct ExtraViews
        {
            VkImageView view;
            u32 mip;
        };

        struct ImageHandlerVKData : IImageHandlerVKData
        {
            std::vector<Image> images;
            std::vector<DepthImage> depthImages;

            std::unordered_map<u16, std::vector<ExtraViews>> extraViews;
        };

        void ImageHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;
            _data = new ImageHandlerVKData();
        }

        void ImageHandlerVK::OnWindowResize()
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Recreate color images
            for (auto& image : data.images)
            {
                if (image.desc.dimensionType == ImageDimensionType::DIMENSION_SCALE)
                {
                    // Destroy old image
                    vkDestroyImageView(_device->_device, image.colorView, nullptr);
                    vmaDestroyImage(_device->_allocator, image.image, image.allocation);
                    
                    // Create new
                    VkFormat format;
                    CreateImage(image, format);
                    CreateImageViews(image, format);
                }
            }

            // Recreate depth images
            for (auto& image : data.depthImages)
            {
                if (image.desc.dimensionType == ImageDimensionType::DIMENSION_SCALE)
                {
                    // Destroy old image
                    vkDestroyImageView(_device->_device, image.depthView, nullptr);
                    vmaDestroyImage(_device->_allocator, image.image, image.allocation);

                    // Create new
                    CreateImage(image);
                }
            }
        }

        ImageID ImageHandlerVK::CreateImage(const ImageDesc& desc)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            size_t nextHandle = data.images.size();

            // Make sure we haven't exceeded the limit of the ImageID type, if this hits you need to change type of ImageID to something bigger
            assert(nextHandle < ImageID::MaxValue());

            Image image;
            image.desc = desc;

            assert(desc.dimensions.x > 0); // Make sure the width is valid
            assert(desc.dimensions.y > 0); // Make sure the height is valid
            assert(desc.depth > 0); // Make sure the depth is valid
            assert(desc.format != ImageFormat::UNKNOWN); // Make sure the format is valid

            VkFormat format;
            CreateImage(image, format);
            CreateImageViews(image, format);

            // Transition image from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_GENERAL
            _device->TransitionImageLayout(image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, image.desc.depth, image.desc.mipLevels);

            data.images.push_back(image);

            return ImageID(static_cast<ImageID::type>(nextHandle));
        }

        ImageID ImageHandlerVK::CreateImageFromSwapchain(const ImageDesc& desc, VkFormat format, VkSwapchainKHR swapChain, u32 imageCount, u32 index)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            size_t nextHandle = data.images.size();

            // Make sure we haven't exceeded the limit of the ImageID type, if this hits you need to change type of ImageID to something bigger
            assert(nextHandle < ImageID::MaxValue());

            Image image;
            image.desc = desc;
            image.isSwapchain = true;

            assert(desc.dimensions.x > 0); // Make sure the width is valid
            assert(desc.dimensions.y > 0); // Make sure the height is valid
            assert(desc.depth > 0); // Make sure the depth is valid
            assert(desc.format != ImageFormat::UNKNOWN); // Make sure the format is valid

            // Get VkImage from swapchain
            VkImage images[3];
            vkGetSwapchainImagesKHR(_device->_device, swapChain, &imageCount, images);
            image.image = images[index];

            CreateImageViews(image, format);

            // Transition image from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_GENERAL
            _device->TransitionImageLayout(image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, image.desc.depth, image.desc.mipLevels);

            data.images.push_back(image);

            return ImageID(static_cast<ImageID::type>(nextHandle));
        }

        DepthImageID ImageHandlerVK::CreateDepthImage(const DepthImageDesc& desc)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            size_t nextHandle = data.depthImages.size();

            // Make sure we haven't exceeded the limit of the DepthImageID type, if this hits you need to change type of DepthImageID to something bigger
            assert(nextHandle < DepthImageID::MaxValue());

            DepthImage image;
            image.desc = desc;

            CreateImage(image);

            data.depthImages.push_back(image);

            return DepthImageID(static_cast<DepthImageID::type>(nextHandle));
        }

        const ImageDesc& ImageHandlerVK::GetImageDesc(const ImageID id)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Lets make sure this id exists
            assert(data.images.size() > static_cast<ImageID::type>(id));
            return data.images[static_cast<ImageID::type>(id)].desc;
        }

        const DepthImageDesc& ImageHandlerVK::GetDepthImageDesc(const DepthImageID id)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Lets make sure this id exists
            assert(data.depthImages.size() > static_cast<DepthImageID::type>(id));
            return data.depthImages[static_cast<DepthImageID::type>(id)].desc;
        }

        uvec2 ImageHandlerVK::GetDimension(const ImageID id, u32 mipLevel)
        {
            auto desc = GetImageDesc(id);

            f32 width = desc.dimensions.x;
            f32 height = desc.dimensions.y;
            u32 mips = desc.mipLevels;

            u32 uwidth = static_cast<u32>(width);;
            u32 uheight = static_cast<u32>(height);

            // If the supplied dimensions is a % of window size
            if (desc.dimensionType == ImageDimensionType::DIMENSION_SCALE)
            {
                uvec2 windowSize = _device->GetMainWindowSize();
                width *= windowSize.x;
                height *= windowSize.y;

                uwidth = static_cast<u32>(width);
                uheight = static_cast<u32>(height);
            }
            else if (desc.dimensionType == ImageDimensionType::DIMENSION_PYRAMID)
            {
                uvec2 windowSize = _device->GetMainWindowSize();
                width *= windowSize.x;
                height *= windowSize.y;

                uwidth = static_cast<u32>(width);
                uheight = static_cast<u32>(height);

                uwidth = (PreviousPow2(uwidth));
                uheight = (PreviousPow2(uheight));
                mips = (GetImageMipLevels(uwidth, uheight));
            }

            u32 mip = glm::min(mips, mipLevel);

            return { uwidth >> mip, uheight >> mip };
        }

        VkImage ImageHandlerVK::GetImage(const ImageID id)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Lets make sure this id exists
            assert(data.images.size() > static_cast<ImageID::type>(id));
            return data.images[static_cast<ImageID::type>(id)].image;
        }

        VkImageView ImageHandlerVK::GetColorView(const ImageID id)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Lets make sure this id exists
            assert(data.images.size() > static_cast<ImageID::type>(id));
            return data.images[static_cast<ImageID::type>(id)].colorView;
        }

        VkImageView ImageHandlerVK::GetColorView(const ImageID id, u32 mipLevel)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);
            VkImageView view = VK_NULL_HANDLE;

            if (mipLevel == 0)
            {
                return GetColorView(id);
            }
            // Lets make sure this id exists
            ImageID::type tid = static_cast<ImageID::type>(id);

            auto img = data.extraViews.find(tid);
            if (img != data.extraViews.end())
            {
                for (auto im : img->second)
                {
                    if (im.mip == mipLevel)
                    {
                        return im.view;
                    }
                }
            }

            return view;
        }

        VkImage ImageHandlerVK::GetImage(const DepthImageID id)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Lets make sure this id exists
            assert(data.depthImages.size() > static_cast<DepthImageID::type>(id));
            return data.depthImages[static_cast<DepthImageID::type>(id)].image;
        }

        VkImageView ImageHandlerVK::GetDepthView(const DepthImageID id)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Lets make sure this id exists
            assert(data.depthImages.size() > static_cast<DepthImageID::type>(id));
            return data.depthImages[static_cast<DepthImageID::type>(id)].depthView;
        }

        bool ImageHandlerVK::IsSwapChainImage(const ImageID id)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Lets make sure this id exists
            assert(data.images.size() > static_cast<ImageID::type>(id));
            return data.images[static_cast<ImageID::type>(id)].isSwapchain;
        }

        u32 ImageHandlerVK::GetNumImages()
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            return static_cast<u32>(data.images.size());
        }

        u32 ImageHandlerVK::GetNumDepthImages()
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            return static_cast<u32>(data.depthImages.size());
        }

        void ImageHandlerVK::CreateImage(Image& image, VkFormat& format)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Create image
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.pNext = nullptr;
            imageInfo.flags = 0; // TODO: VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT and VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT? https://github.com/DiligentGraphics/DiligentCore/blob/1edcafe9bd41bdde86869d4e1c0212c78ce123b7/Graphics/GraphicsEngineVulkan/src/TextureVkImpl.cpp

            if (image.desc.depth == 1)
            {
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
            }
            else
            {
                DebugHandler::PrintFatal("Non-3d images is currently unsupported");
            }
            if (image.desc.dimensionType != ImageDimensionType::DIMENSION_PYRAMID)
            {
                image.desc.mipLevels = 1;
            }

            f32 width = image.desc.dimensions.x;
            f32 height = image.desc.dimensions.y;
            u32 mips = image.desc.mipLevels;

            u32 uwidth = static_cast<u32>(width);;
            u32 uheight = static_cast<u32>(height);

            // If the supplied dimensions is a % of window size
            if (image.desc.dimensionType == ImageDimensionType::DIMENSION_SCALE)
            {
                uvec2 windowSize = _device->GetMainWindowSize();
                width *= windowSize.x;
                height *= windowSize.y;

                uwidth = static_cast<u32>(width);
                uheight = static_cast<u32>(height);
            }
            else if (image.desc.dimensionType == ImageDimensionType::DIMENSION_PYRAMID)
            {
                uvec2 windowSize = _device->GetMainWindowSize();
                width *= windowSize.x;
                height *= windowSize.y;

                uwidth = static_cast<u32>(width);
                uheight = static_cast<u32>(height);

                uwidth = (PreviousPow2(uwidth));
                uheight = (PreviousPow2(uheight));
                mips = (GetImageMipLevels(uwidth, uheight));

                image.desc.mipLevels = mips;
            }

            format = FormatConverterVK::ToVkFormat(image.desc.format);

            imageInfo.format = format;
            imageInfo.extent.width = uwidth;
            imageInfo.extent.height = uheight;
            imageInfo.extent.depth = image.desc.depth;
            imageInfo.mipLevels = mips;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = FormatConverterVK::ToVkSampleCount(image.desc.sampleCount);
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

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            if (vmaCreateImage(_device->_allocator, &imageInfo, &allocInfo, &image.image, &image.allocation, nullptr) != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to create image!");
            }
        }

        void ImageHandlerVK::CreateImage(DepthImage& image)
        {
            // Create image
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.pNext = nullptr;
            imageInfo.flags = 0; // TODO: VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT and VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT? https://github.com/DiligentGraphics/DiligentCore/blob/1edcafe9bd41bdde86869d4e1c0212c78ce123b7/Graphics/GraphicsEngineVulkan/src/TextureVkImpl.cpp
            imageInfo.imageType = VK_IMAGE_TYPE_2D;

            f32 width = image.desc.dimensions.x;
            f32 height = image.desc.dimensions.y;

            // If the supplied dimensions is a % of window size
            if (image.desc.dimensionType == ImageDimensionType::DIMENSION_SCALE)
            {
                uvec2 windowSize = _device->GetMainWindowSize();
                width *= windowSize.x;
                height *= windowSize.y;
            }

            imageInfo.format = FormatConverterVK::ToVkFormat(image.desc.format);
            imageInfo.extent.width = static_cast<u32>(width);
            imageInfo.extent.height = static_cast<u32>(height);
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = FormatConverterVK::ToVkSampleCount(image.desc.sampleCount);
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

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            if (vmaCreateImage(_device->_allocator, &imageInfo, &allocInfo, &image.image, &image.allocation, nullptr) != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to create image!");
            }

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

            if (vkCreateImageView(_device->_device, &depthViewInfo, nullptr, &image.depthView) != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to create depth image view!");
            }

            DebugMarkerUtilVK::SetObjectName(_device->_device, (u64)image.depthView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, image.desc.debugName.c_str());

            // Transition image from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            _device->TransitionImageLayout(image.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1, 1);
        }

        void ImageHandlerVK::CreateImageViews(Image& image, VkFormat format)
        {
            ImageHandlerVKData& data = static_cast<ImageHandlerVKData&>(*_data);

            // Create Color View
            VkImageViewCreateInfo colorViewInfo = {};
            colorViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            colorViewInfo.image = image.image;
            if (image.desc.depth == 1)
            {
                colorViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            }
            else
            {
                DebugHandler::PrintFatal("Non-3d images is currently unsupported");
            }

            colorViewInfo.format = format;
            colorViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
            colorViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorViewInfo.subresourceRange.baseMipLevel = 0;
            colorViewInfo.subresourceRange.levelCount = image.desc.mipLevels;
            colorViewInfo.subresourceRange.baseArrayLayer = 0;
            colorViewInfo.subresourceRange.layerCount = 1;

            //we want a full mip chain of views
            if (image.desc.dimensionType == ImageDimensionType::DIMENSION_PYRAMID)
            {
                std::vector<ExtraViews> views;

                for (u32 i = 0; i < image.desc.mipLevels; ++i)
                {
                    VkImageViewCreateInfo pyramidLevelInfo = colorViewInfo;
                    pyramidLevelInfo.subresourceRange.baseMipLevel = i;
                    pyramidLevelInfo.subresourceRange.levelCount = 1;

                    VkImageView newView;
                    if (vkCreateImageView(_device->_device, &pyramidLevelInfo, nullptr, &newView) != VK_SUCCESS)
                    {
                        DebugHandler::PrintFatal("Failed to create color image view!");
                    }
                    ExtraViews v;
                    v.view = newView;
                    v.mip = i;
                    views.push_back(v);
                }
                
                data.extraViews[(static_cast<u16>(data.images.size()))] = std::move(views);
            }

            if (vkCreateImageView(_device->_device, &colorViewInfo, nullptr, &image.colorView) != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to create color image view!");
            }

            DebugMarkerUtilVK::SetObjectName(_device->_device, (u64)image.colorView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, image.desc.debugName.c_str());
        }
    }
}