#include "BufferBackendVK.h"
#include "RenderDeviceVK.h"

namespace Renderer
{
    namespace Backend
    {
        void BufferBackendVK::Apply(u32 frameIndex, void* data, size_t size)
        {
            void* destData;
            vmaMapMemory(device->_allocator, allocations.Get(frameIndex), &destData);
            memcpy(destData, data, size);
            vmaUnmapMemory(device->_allocator, allocations.Get(frameIndex));
        }

        void* BufferBackendVK::GetDescriptor(u32 frameIndex)
        {
            return static_cast<void*>(this);
        }

        void* BufferBackendVK::GetBuffer(u32 frameIndex)
        {
            return static_cast<void*>(&buffers.Get(frameIndex));
        }
    }
}
