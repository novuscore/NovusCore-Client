#include "DescriptorAllocatorVK.h"
#include <Utils/DebugHandler.h>
#include "RenderDeviceVK.h"
#include <vector>
#include <memory>
#include <mutex>

namespace Renderer
{
    namespace Backend
    {
        bool IsMemoryError(VkResult result)
        {
            switch (result)
            {
            case VK_ERROR_FRAGMENTED_POOL:
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                return true;
            }
            return false;
        }

        struct DescriptorAllocatorVK
        {
            VkDescriptorPool pool;
        };

        struct PoolStorage
        {
            std::vector<DescriptorAllocatorVK> _usableAllocators;
            std::vector<DescriptorAllocatorVK> _fullAllocators;
        };

        struct PoolSize
        {
            VkDescriptorType type;
            f32 multiplier;
        };

        struct PoolSizes
        {
            std::vector<PoolSize> sizes =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 4.0f},
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f},
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10.0f},
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.0f},
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.0f},
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.0f},
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.0f},
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.0f},
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.0f},
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.0f},
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1.0f}
            };
        };

        class DescriptorAllocatorPoolVKImpl : public DescriptorAllocatorPoolVK
        {
        public:
            virtual ~DescriptorAllocatorPoolVKImpl();

            void SetPoolSizeMultiplier(VkDescriptorType type, f32 multiplier) override final;
            void Flip() override final;
            DescriptorAllocatorHandleVK GetAllocator() override final;

            void ReturnAllocator(DescriptorAllocatorHandleVK& handle, bool isFull);
            VkDescriptorPool CreatePool(i32 count, VkDescriptorPoolCreateFlags flags);

            RenderDeviceVK* _device;
            PoolSizes _poolSizes;

            i32 _frameIndex;
            i32 _numFrames;

            std::mutex _poolMutex;
            
            // Zero is for static pool, next is for frame indexing
            std::vector<std::unique_ptr<PoolStorage>> _descriptorPools;

            // Fully cleared allocators
            std::vector<DescriptorAllocatorVK> _clearAllocators;
        };

        DescriptorAllocatorPoolVK* DescriptorAllocatorPoolVK::Create(RenderDeviceVK* device, i32 numFrames)
        {
            DescriptorAllocatorPoolVKImpl* impl = new DescriptorAllocatorPoolVKImpl();

            impl->_device = device;
            impl->_frameIndex = 0;
            impl->_numFrames = numFrames;
            
            for (i32 i = 0; i < numFrames; i++)
            {
                impl->_descriptorPools.push_back(std::make_unique<PoolStorage>());
            }

            return impl;
        }

        DescriptorAllocatorHandleVK::~DescriptorAllocatorHandleVK()
        {
            DescriptorAllocatorPoolVKImpl* implPool = static_cast<DescriptorAllocatorPoolVKImpl*>(ownerPool);
            if (implPool)
            {
                implPool->ReturnAllocator(*this, false);
            }
        }

        DescriptorAllocatorHandleVK::DescriptorAllocatorHandleVK(DescriptorAllocatorHandleVK&& other)
        {
            Return();

            vkPool = other.vkPool;
            poolID = other.poolID;
            ownerPool = other.ownerPool;

            other.ownerPool = nullptr;
            other.poolID = -1;
            other.vkPool = VkDescriptorPool();
        }

        DescriptorAllocatorHandleVK& DescriptorAllocatorHandleVK::operator=(DescriptorAllocatorHandleVK&& other)
        {
            Return();

            vkPool = other.vkPool;
            poolID = other.poolID;
            ownerPool = other.ownerPool;

            other.ownerPool = nullptr;
            other.poolID = -1;
            other.vkPool = VkDescriptorPool();

            return *this;
        }

        void DescriptorAllocatorHandleVK::Return()
        {
            DescriptorAllocatorPoolVKImpl* implPool = static_cast<DescriptorAllocatorPoolVKImpl*>(ownerPool);
            if (implPool)
            {
                implPool->ReturnAllocator(*this, false);
            }

            vkPool = VkDescriptorPool{};
            poolID = -1;
            ownerPool = nullptr;
        }

        bool DescriptorAllocatorHandleVK::Allocate(const VkDescriptorSetLayout& layout, VkDescriptorSet& set, void* next)
        {
            DescriptorAllocatorPoolVKImpl* implPool = static_cast<DescriptorAllocatorPoolVKImpl*>(ownerPool);

            VkDescriptorSetAllocateInfo allocInfo;
            allocInfo.pNext = next;
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = vkPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &layout;

            VkResult result = vkAllocateDescriptorSets(implPool->_device->_device, &allocInfo, &set);
            if (result != VK_SUCCESS)
            {
                // We allocate pools on memory error
                if (IsMemoryError(result))
                {
                    // Out of space, we need to reallocate
                    implPool->ReturnAllocator(*this, true);

                    DescriptorAllocatorHandleVK newHandle = implPool->GetAllocator();

                    vkPool = newHandle.vkPool;
                    poolID = newHandle.poolID;

                    newHandle.vkPool = VkDescriptorPool{};
                    newHandle.poolID = -1;
                    newHandle.ownerPool = nullptr;

                    return Allocate(layout, set);
                }
                else
                {
                    NC_LOG_FATAL("Could not allocate descriptor set");
                    return false;
                }
            }

            return true;
        }

        VkDescriptorPool DescriptorAllocatorPoolVKImpl::CreatePool(i32 count, VkDescriptorPoolCreateFlags flags)
        {
            std::vector<VkDescriptorPoolSize> sizes;
            sizes.reserve(_poolSizes.sizes.size());
            
            for (auto size : _poolSizes.sizes)
            {
                sizes.push_back({ size.type, u32(size.multiplier * count) });
            }

            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.flags = flags;
            poolInfo.maxSets = count;
            poolInfo.poolSizeCount = static_cast<u32>(sizes.size());
            poolInfo.pPoolSizes = sizes.data();

            VkDescriptorPool descriptorPool;
            VkResult result = vkCreateDescriptorPool(_device->_device, &poolInfo, nullptr, &descriptorPool);

            if (result != VK_SUCCESS)
            {
                NC_LOG_FATAL("Could not create descriptor pool");
            }

            return descriptorPool;
        }

        DescriptorAllocatorPoolVKImpl::~DescriptorAllocatorPoolVKImpl()
        {
            for (DescriptorAllocatorVK allocator : _clearAllocators)
            {
                vkDestroyDescriptorPool(_device->_device, allocator.pool, nullptr);
            }
            for (auto&& storage : _descriptorPools)
            {
                for (DescriptorAllocatorVK allocator : storage->_fullAllocators)
                {
                    vkDestroyDescriptorPool(_device->_device, allocator.pool, nullptr);
                }
                for (DescriptorAllocatorVK allocator : storage->_usableAllocators)
                {
                    vkDestroyDescriptorPool(_device->_device, allocator.pool, nullptr);
                }
            }
        }

        void DescriptorAllocatorPoolVKImpl::Flip()
        {
            _frameIndex = (_frameIndex + 1) % _numFrames;

            for (auto allocator : _descriptorPools[_frameIndex]->_fullAllocators)
            {
                vkResetDescriptorPool(_device->_device, allocator.pool, VkDescriptorPoolResetFlags{ 0 });

                _clearAllocators.push_back(allocator);
            }

            for (auto allocator : _descriptorPools[_frameIndex]->_usableAllocators)
            {
                vkResetDescriptorPool(_device->_device, allocator.pool, VkDescriptorPoolResetFlags{ 0 });

                _clearAllocators.push_back(allocator);
            }

            _descriptorPools[_frameIndex]->_fullAllocators.clear();
            _descriptorPools[_frameIndex]->_usableAllocators.clear();
        }

        void DescriptorAllocatorPoolVKImpl::SetPoolSizeMultiplier(VkDescriptorType type, f32 multiplier)
        {
            for (auto& size : _poolSizes.sizes)
            {
                if (size.type == type)
                {
                    size.multiplier = multiplier;
                    return;
                }
            }

            // Not found, so add it
            PoolSize newSize;
            newSize.type = type;
            newSize.multiplier = multiplier;
            _poolSizes.sizes.push_back(newSize);
        }

        void DescriptorAllocatorPoolVKImpl::ReturnAllocator(DescriptorAllocatorHandleVK& handle, bool isFull)
        {
            std::lock_guard<std::mutex> lock(_poolMutex);

            if (isFull)
            {
                _descriptorPools[handle.poolID]->_fullAllocators.push_back(DescriptorAllocatorVK{ handle.vkPool });
            }
            else
            {
                _descriptorPools[handle.poolID]->_usableAllocators.push_back(DescriptorAllocatorVK{ handle.vkPool });
            }
        }

        DescriptorAllocatorHandleVK DescriptorAllocatorPoolVKImpl::GetAllocator()
        {
            std::lock_guard<std::mutex> lock(_poolMutex);

            bool foundAllocator = false;
            i32 poolIndex = _frameIndex;

            DescriptorAllocatorVK allocator;

            // Try to reuse an allocated pool
            if (_clearAllocators.size() != 0)
            {
                allocator = _clearAllocators.back();
                _clearAllocators.pop_back();
                foundAllocator = true;
            }
            else
            {
                if (_descriptorPools[poolIndex]->_usableAllocators.size() > 0)
                {
                    allocator = _descriptorPools[poolIndex]->_usableAllocators.back();
                    _descriptorPools[poolIndex]->_usableAllocators.pop_back();
                    foundAllocator = true;
                }
            }

            // Need a new pool
            if (!foundAllocator)
            {
                // Static pool has to be free-able
                VkDescriptorPoolCreateFlags flags = 0;
                if (poolIndex == 0)
                {
                    flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                }

                VkDescriptorPool newPool = CreatePool(2000, flags);
                allocator.pool = newPool;

                foundAllocator = true;
            }

            DescriptorAllocatorHandleVK newHandle;
            newHandle.ownerPool = this;
            newHandle.poolID = poolIndex;
            newHandle.vkPool = allocator.pool;

            return newHandle;
        }

    }
}