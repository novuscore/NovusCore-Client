#pragma once
#include <NovusTypes.h>

#include "../../../Descriptors/BufferDesc.h"

#include "vk_mem_alloc.h"
#include "vulkan/vulkan_core.h"

#include <vector>
#include <queue>

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

            void OnFrameStart();

            VkBuffer GetBuffer(BufferID bufferID) const;
            VkDeviceSize GetBufferSize(BufferID bufferID) const;
            VmaAllocation GetBufferAllocation(BufferID bufferID) const;

            BufferID CreateBuffer(BufferDesc& desc);
            BufferID CreateTemporaryBuffer(BufferDesc& desc, u32 framesLifetime);
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

            struct TemporaryBuffer
            {
                BufferID bufferID;
                u32 framesLifetimeLeft;
            };

            std::vector<Buffer> _buffers;
            std::queue<BufferID> _returnedBufferIDs;

            std::vector<TemporaryBuffer> _temporaryBuffers;

            friend class RendererVK;
        };
    }
}