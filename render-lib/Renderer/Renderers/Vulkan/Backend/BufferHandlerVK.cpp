#include "BufferHandlerVK.h"
#include "RenderDeviceVK.h"

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

        void BufferHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;
        }

        VkBuffer BufferHandlerVK::GetBuffer(BufferID bufferID) const
        {
            return _buffers[static_cast<BufferID::type>(bufferID)].buffer;
        }

        VkDeviceSize BufferHandlerVK::GetBufferSize(BufferID bufferID) const
        {
            return _buffers[static_cast<BufferID::type>(bufferID)].size;
        }

        VmaAllocation BufferHandlerVK::GetBufferAllocation(BufferID bufferID) const
        {
            return _buffers[static_cast<BufferID::type>(bufferID)].allocation;
        }

        BufferID BufferHandlerVK::CreateBuffer(BufferDesc& desc)
        {
            VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

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
                memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            }

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = memoryUsage;

            const size_t nextHandle = _buffers.size();

            // Make sure we haven't exceeded the limit of the ImageID type, if this hits you need to change type of ImageID to something bigger
            assert(nextHandle < BufferID::MaxValue());
            
            BufferID bufferID(static_cast<BufferID::type>(nextHandle));
            Buffer& buffer = _buffers.emplace_back();
            buffer.size = desc.size;

            if (vmaCreateBuffer(_device->_allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create buffer!");
            }

            return bufferID;
        }
    }
}
