#include "ConstantBufferVK.h"
#include "RenderDeviceVK.h"

namespace Renderer
{
    namespace Backend
    {
        void ConstantBufferBackendVK::Apply(u32 frameIndex, void* data, size_t size)
        {
            void* destData;
            vkMapMemory(device->_device, uniformBuffersMemory.Get(frameIndex), 0, size, 0, &destData);
            memcpy(destData, data, size);
            vkUnmapMemory(device->_device, uniformBuffersMemory.Get(frameIndex));
        }

        void* ConstantBufferBackendVK::GetGPUResource(u32 frameIndex)
        {
            return static_cast<void*>(this);
        }
    }
}
