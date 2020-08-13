#include "BufferHandlerVK.h"
#include "RenderDeviceVK.h"
#include "DebugMarkerUtilVK.h"

#include "vulkan/vulkan.h"

constexpr size_t MaxBufferCount = 4096;

static_assert(MaxBufferCount <= std::numeric_limits<Renderer::BufferID::type>::max(), "Too many buffers to fit inside BufferID");

namespace Renderer
{
    namespace Backend
    {
        BufferHandlerVK::BufferHandlerVK()
        {
            _buffers = new Buffer[MaxBufferCount];
            _indices = new Index[MaxBufferCount];

            _bufferCount = 0;

            for (unsigned i = 0; i < MaxBufferCount; ++i) 
            {
                _indices[i].next = i + 1;
            }

            _freelistDequeue = 0;
            _freelistEnqueue = MaxBufferCount - 1;
        }

        BufferHandlerVK::~BufferHandlerVK()
        {
            assert(_bufferCount == 0);

            delete[] _buffers;
            delete[] _indices;
        }

        BufferID BufferHandlerVK::AcquireNewBufferID() 
        {
            assert(_bufferCount < MaxBufferCount);
            ++_bufferCount;

            BufferID bufferID = BufferID((BufferID::type)_freelistDequeue);
            const Index& in = _indices[_freelistDequeue];
            _freelistDequeue = in.next;
            return bufferID;
        }

        void BufferHandlerVK::ReturnBufferID(BufferID bufferID)
        {
            _indices[_freelistEnqueue].next = (BufferID::type)bufferID;
            _freelistEnqueue = (BufferID::type)bufferID;

            --_bufferCount;
        }

        void BufferHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;
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

        void BufferHandlerVK::DestroyBuffer(BufferID bufferID)
        {
            Buffer& buffer = _buffers[(BufferID::type)bufferID];

            vmaDestroyBuffer(_device->_allocator, buffer.buffer, buffer.allocation);

            ReturnBufferID(bufferID);
        }
    }
}
