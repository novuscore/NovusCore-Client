#include "BufferHandlerVK.h"
#include "RenderDeviceVK.h"
#include "DebugMarkerUtilVK.h"

#include "vulkan/vulkan.h"

namespace Renderer
{
    namespace Backend
    {
        BufferHandlerVK::BufferHandlerVK()
        {
        }

        BufferHandlerVK::~BufferHandlerVK()
        {
        }

        BufferID BufferHandlerVK::AcquireNewBufferID() 
        {
            BufferID bufferID;

            // Check if we have returned bufferIDs to use
            if (_returnedBufferIDs.size() > 0)
            {
                bufferID = _returnedBufferIDs.front();
                _returnedBufferIDs.pop();
            }
            else
            {
                // Else create a new one
                bufferID = BufferID(static_cast<BufferID::type>(_buffers.size()));
                _buffers.emplace_back();
            }

            return bufferID;
        }

        void BufferHandlerVK::ReturnBufferID(BufferID bufferID)
        {
            _returnedBufferIDs.push(bufferID);
        }

        void BufferHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;
        }

        void BufferHandlerVK::OnFrameStart()
        {
            i64 numBuffers = static_cast<i64>(_temporaryBuffers.size());

            if (numBuffers > 0)
            {
                for (i64 i = numBuffers - 1; i >= 0; i--)
                {
                    TemporaryBuffer& buffer = _temporaryBuffers[i];

                    if (--buffer.framesLifetimeLeft == 0)
                    {
                        DestroyBuffer(buffer.bufferID);
                        _temporaryBuffers.erase(_temporaryBuffers.begin() + i);
                    }
                }
            }
        }

        VkBuffer BufferHandlerVK::GetBuffer(BufferID bufferID) const
        {
            assert(bufferID != BufferID::Invalid());
            return _buffers[static_cast<BufferID::type>(bufferID)].buffer;
        }

        VkDeviceSize BufferHandlerVK::GetBufferSize(BufferID bufferID) const
        {
            assert(bufferID != BufferID::Invalid());
            return _buffers[static_cast<BufferID::type>(bufferID)].size;
        }

        VmaAllocation BufferHandlerVK::GetBufferAllocation(BufferID bufferID) const
        {
            assert(bufferID != BufferID::Invalid());
            return _buffers[static_cast<BufferID::type>(bufferID)].allocation;
        }

        BufferID BufferHandlerVK::CreateBuffer(BufferDesc& desc)
        {
            VkBufferUsageFlags usage = 0;

            if (desc.usage & BUFFER_USAGE_VERTEX_BUFFER)
            {
                usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }

            if (desc.usage & BUFFER_USAGE_INDEX_BUFFER)
            {
                usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }

            if (desc.usage & BUFFER_USAGE_UNIFORM_BUFFER)
            {
                usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }
            
            if (desc.usage & BUFFER_USAGE_STORAGE_BUFFER)
            {
                usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }

            if (desc.usage & BUFFER_USAGE_INDIRECT_ARGUMENT_BUFFER)
            {
                usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            if (desc.usage & BUFFER_USAGE_TRANSFER_SOURCE)
            {
                usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            }

            if (desc.usage & BUFFER_USAGE_TRANSFER_DESTINATION)
            {
                usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            }

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = desc.size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            
            VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
            if (desc.cpuAccess == BufferCPUAccess::ReadOnly)
            {
                memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
            }
            else if (desc.cpuAccess == BufferCPUAccess::WriteOnly)
            {
                memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
            }

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = memoryUsage;

            const BufferID bufferID = AcquireNewBufferID();
            Buffer& buffer = _buffers[(BufferID::type)bufferID];
            buffer.size = desc.size;

            if (vmaCreateBuffer(_device->_allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create buffer!");
                return BufferID::Invalid();
            }

            DebugMarkerUtilVK::SetObjectName(_device->_device, (u64)buffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, desc.name.c_str());

            return bufferID;
        }

        BufferID BufferHandlerVK::CreateTemporaryBuffer(BufferDesc& desc, u32 framesLifetime)
        {
            TemporaryBuffer& temporaryBuffer = _temporaryBuffers.emplace_back();
            temporaryBuffer.bufferID = CreateBuffer(desc);
            temporaryBuffer.framesLifetimeLeft = framesLifetime;

            return temporaryBuffer.bufferID;
        }

        void BufferHandlerVK::DestroyBuffer(BufferID bufferID)
        {
            Buffer& buffer = _buffers[(BufferID::type)bufferID];

            vmaDestroyBuffer(_device->_allocator, buffer.buffer, buffer.allocation);

            ReturnBufferID(bufferID);
        }
    }
}
