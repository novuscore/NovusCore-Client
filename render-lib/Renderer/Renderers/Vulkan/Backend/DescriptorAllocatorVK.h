#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan_core.h>

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        class DescriptorAllocatorPoolVK;

        struct DescriptorAllocatorHandleVK
        {
            friend struct DescriptorAllocatorVK;
            DescriptorAllocatorHandleVK() = default;
            DescriptorAllocatorHandleVK& operator=(const DescriptorAllocatorHandleVK&) = delete;

            ~DescriptorAllocatorHandleVK();
            DescriptorAllocatorHandleVK(DescriptorAllocatorHandleVK&& other);
            DescriptorAllocatorHandleVK& operator=(DescriptorAllocatorHandleVK&& other);

            // Return this handle to the pool, will make this handle orphaned
            void Return();

            // Allocate new descriptor, handle has to be valid
            // Will mutate the handle if it requires a new vkDescriptorPool
            bool Allocate(const VkDescriptorSetLayout& layout, VkDescriptorSet& set, void* pNext = nullptr);

            DescriptorAllocatorPoolVK* ownerPool{ nullptr };
            VkDescriptorPool vkPool;
            i8 poolID;
        };

        class DescriptorAllocatorPoolVK
        {
        public:
            static DescriptorAllocatorPoolVK* Create(RenderDeviceVK* device, i32 numFrames = 3);

            virtual void SetPoolSizeMultiplier(VkDescriptorType type, f32 multiplier) = 0;
            
            virtual void Flip() = 0;
            virtual DescriptorAllocatorHandleVK GetAllocator() = 0;
        };
    }
}