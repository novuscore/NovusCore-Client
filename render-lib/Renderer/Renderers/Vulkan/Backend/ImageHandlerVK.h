#pragma once
#include <NovusTypes.h>
#include <vector>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#include "../../../Descriptors/ImageDesc.h"
#include "../../../Descriptors/DepthImageDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        class ImageHandlerVK
        {
        public:
            void Init(RenderDeviceVK* device);

            void OnWindowResize();

            ImageID CreateImage(const ImageDesc& desc);
            DepthImageID CreateDepthImage(const DepthImageDesc& desc);

            const ImageDesc& GetImageDesc(const ImageID id);
            const DepthImageDesc& GetDepthImageDesc(const DepthImageID id);

            VkImage GetImage(const ImageID id);
            VkImageView GetColorView(const ImageID id);

            VkImage GetImage(const DepthImageID id);
            VkImageView GetDepthView(const DepthImageID id);

        private:
            struct Image
            {
                ImageDesc desc;

                VmaAllocation allocation;
                VkImage image;
                VkImageView colorView;
            };

            struct DepthImage
            {
                DepthImageDesc desc;

                VmaAllocation allocation;
                VkImage image;
                VkImageView depthView;
            };

            void CreateImage(Image& image);
            void CreateImage(DepthImage& image);

        private:
            RenderDeviceVK* _device;

            std::vector<Image> _images;
            std::vector<DepthImage> _depthImages;
        };
    }
}