#pragma once
#include <NovusTypes.h>

#include "../../../Descriptors/BufferDesc.h"

#include "vk_mem_alloc.h"
#include "vulkan/vulkan_core.h"

#include <vector>

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        class BufferHandlerVK
        {
        public:
            BufferHandlerVK();
            ~BufferHandlerVK();

            void Init(RenderDeviceVK* device);

            VkBuffer GetBuffer(BufferID bufferID) const;
            VkDeviceSize GetBufferSize(BufferID bufferID) const;
            VmaAllocation GetBufferAllocation(BufferID bufferID) const;

            BufferID CreateBuffer(BufferDesc& desc);

        private:
            RenderDeviceVK* _device;

            struct Buffer
            {
                VmaAllocation allocation;
                VkBuffer buffer;
                VkDeviceSize size;
            };

            std::vector<Buffer> _buffers;

            friend class RendererVK;
        };
    }
}