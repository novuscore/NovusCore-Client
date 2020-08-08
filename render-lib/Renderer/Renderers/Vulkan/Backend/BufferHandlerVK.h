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
            void DestroyBuffer(BufferID bufferID);

        private:
            BufferID AcquireNewBufferID();
            void ReturnBufferID(BufferID bufferID);

            RenderDeviceVK* _device = nullptr;

            struct Buffer
            {
                VmaAllocation allocation;
                VkBuffer buffer;
                VkDeviceSize size;
            };

            struct Index {
                u32 next;
            };

            u32 _bufferCount;
            Buffer* _buffers = nullptr;
            Index* _indices = nullptr;
            u32 _freelistEnqueue;
            u32 _freelistDequeue;

            friend class RendererVK;
        };
    }
}