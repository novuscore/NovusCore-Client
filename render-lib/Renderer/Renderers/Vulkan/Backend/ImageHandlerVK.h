#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan_core.h>

#include "vk_mem_alloc.h"
#include "../../../Descriptors/ImageDesc.h"
#include "../../../Descriptors/DepthImageDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        struct Image;
        struct DepthImage;

        struct IImageHandlerVKData {};

        class ImageHandlerVK
        {
        public:
            void Init(RenderDeviceVK* device);

            void OnWindowResize();

            ImageID CreateImage(const ImageDesc& desc);
            ImageID CreateImageFromSwapchain(const ImageDesc& desc, VkFormat format, VkSwapchainKHR swapChain, u32 imageCount, u32 index);

            DepthImageID CreateDepthImage(const DepthImageDesc& desc);

            const ImageDesc& GetImageDesc(const ImageID id);
            const DepthImageDesc& GetDepthImageDesc(const DepthImageID id);

            VkImage GetImage(const ImageID id);
            VkImageView GetColorView(const ImageID id);
            VkImageView GetColorView(const ImageID id, u32 mipLevel);

            uvec2 GetDimension(const ImageID id, u32 mipLevel);

            VkImage GetImage(const DepthImageID id);
            VkImageView GetDepthView(const DepthImageID id);

            bool IsSwapChainImage(const ImageID id);

            u32 GetNumImages();
            u32 GetNumDepthImages();

        private:
            void CreateImage(Image& image, VkFormat& format);
            void CreateImage(DepthImage& image);

            void CreateImageViews(Image& image, VkFormat format);

        private:
            RenderDeviceVK* _device;

            IImageHandlerVKData* _data;
        };
    }
}