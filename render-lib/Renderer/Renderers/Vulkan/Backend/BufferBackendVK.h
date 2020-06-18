#pragma once
#include <NovusTypes.h>
#include "../../../BufferBackend.h"
#include "../../../FrameResource.h"
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        struct BufferBackendVK : public BufferBackend
        {
            BufferBackendVK() {};
            ~BufferBackendVK()
            {

            }

            RenderDeviceVK* device;

            FrameResource<VkBuffer, 2> buffers;
            FrameResource<VmaAllocation, 2> allocations;

            VkDescriptorPool descriptorPool = 0;
            FrameResource<VkDescriptorSet, 2> descriptorSet;

            size_t bufferSize;
            BufferBackend::Type type;
        private:
            void Apply(u32 frameIndex, void* data, size_t size) override;

            void* GetDescriptor(u32 frameIndex) override;
            void* GetBuffer(u32 frameIndex) override;
        };
    }
}