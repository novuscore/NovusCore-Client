#pragma once
#include <NovusTypes.h>
#include "../../../ConstantBuffer.h"
#include "../../../FrameResource.h"
#include <vulkan/vulkan.h>

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        struct ConstantBufferBackendVK : public ConstantBufferBackend
        {
            ConstantBufferBackendVK() {};
            ~ConstantBufferBackendVK()
            {

            }

            RenderDeviceVK* device;

            FrameResource<VkBuffer, 2> uniformBuffers;
            FrameResource<VkDeviceMemory, 2> uniformBuffersMemory;

            VkDescriptorPool descriptorPool = 0;
            FrameResource<VkDescriptorSet, 2> descriptorSet;

            size_t bufferSize;
        private:
            void Apply(u32 frameIndex, void* data, size_t size) override;

            void* GetGPUResource(u32 frameIndex) override;
        };
    }
}