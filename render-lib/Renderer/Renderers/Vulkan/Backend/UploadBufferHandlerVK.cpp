#include "UploadBufferHandlerVK.h"
#include "BufferHandlerVK.h"
#include "TextureHandlerVK.h"
#include "CommandListHandlerVK.h"
#include "SemaphoreHandlerVK.h"
#include "RenderDeviceVK.h"

#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include <Utils/ConcurrentQueue.h>
#include <Utils/SafeVector.h>

namespace Renderer
{
    namespace Backend
    {
        struct UploadToBufferTask
        {
            BufferID targetBuffer;
            size_t targetOffset;
            size_t stagingBufferOffset;
            size_t copySize;
        };

        struct UploadToTextureTask
        {
            TextureID targetTexture = TextureID::Invalid();
            size_t targetOffset;
            size_t stagingBufferOffset;
        };

        struct StagingBuffer
        {
            BufferID buffer = BufferID::Invalid();
            void* mappedMemory = nullptr;
            Memory::StackAllocator allocator;

            moodycamel::ConcurrentQueue<UploadToBufferTask> uploadToBufferTasks;
            moodycamel::ConcurrentQueue<UploadToTextureTask> uploadToTextureTasks;

            bool isFirstThreadToReach = true;
            
            std::atomic<bool> isSubmitted = false;

            std::mutex handleLock;
            i32 activeHandles = 0;
            bool isFull = false;

            VkFence fence;
        };

        struct UploadBufferHandlerVKData : IUploadBufferHandlerVKData
        {
            FrameResource<StagingBuffer, 3> stagingBuffers;
            std::atomic<u32> selectedStagingBuffer = 0;

            bool isDirty = true;
            SemaphoreID uploadFinishedSemaphore;

            std::mutex submitMutex;
        };

        void UploadBufferHandlerVK::Init(RenderDeviceVK* device, BufferHandlerVK* bufferHandler, TextureHandlerVK* textureHandler, SemaphoreHandlerVK* semaphoreHandler, CommandListHandlerVK* commandListHandler)
        {
            _device = device;
            _bufferHandler = bufferHandler;
            _textureHandler = textureHandler;
            _semaphoreHandler = semaphoreHandler;
            _commandListHandler = commandListHandler;
            _data = new UploadBufferHandlerVKData();

            UploadBufferHandlerVKData* data = static_cast<UploadBufferHandlerVKData*>(_data);
            for (u32 i = 0; i < data->stagingBuffers.Num; i++)
            {
                StagingBuffer& stagingBuffer = data->stagingBuffers.Get(i);

                BufferDesc bufferDesc;
                bufferDesc.name = "StagingBuffer" + std::to_string(i);
                bufferDesc.size = BUFFER_SIZE;
                bufferDesc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;
                bufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

                stagingBuffer.buffer = _bufferHandler->CreateBuffer(bufferDesc);
                stagingBuffer.allocator.Init(BUFFER_SIZE, "StagingBuffer", true, false);

                // Map the buffer
                void* mappedStagingMemory;
                VkResult result = vmaMapMemory(_device->_allocator, _bufferHandler->GetBufferAllocation(stagingBuffer.buffer), &mappedStagingMemory);
                if (result != VK_SUCCESS)
                {
                    DebugHandler::PrintFatal("vmaMapMemory failed!\n");
                }
                stagingBuffer.mappedMemory = mappedStagingMemory;

                // Create fence
                VkFenceCreateInfo fenceInfo = {};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

                vkCreateFence(_device->_device, &fenceInfo, nullptr, &stagingBuffer.fence);
            }

            data->uploadFinishedSemaphore = _semaphoreHandler->CreateNSemaphore();
        }

        void UploadBufferHandlerVK::ExecuteUploadTasks()
        {
            UploadBufferHandlerVKData* data = static_cast<UploadBufferHandlerVKData*>(_data);

            if (!data->isDirty)
                return;

            ZoneScoped;

            CommandListID commandListID = _commandListHandler->BeginCommandList(QueueType::Graphics);

            VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

#if TRACY_ENABLE
            TracySourceLocation(ExecuteUpload, "ExecuteUpload", tracy::Color::Yellow2);
            tracy::VkCtxManualScope tracyScope(_device->_tracyContext, &ExecuteUpload, true);
            tracyScope.Start(commandBuffer);
#endif

            for (u32 i = 0; i < data->stagingBuffers.Num; i++)
            {
                StagingBuffer& stagingBuffer = data->stagingBuffers.Get(i);

                if (stagingBuffer.isSubmitted)
                {
                    WaitForStagingBuffer(stagingBuffer);
                }
                else
                {
                    ExecuteStagingBuffer(commandBuffer, stagingBuffer);
                }
            }

#if TRACY_ENABLE
            tracyScope.End();
#endif

            VkSemaphore semaphore = _semaphoreHandler->GetVkSemaphore(data->uploadFinishedSemaphore);
            _commandListHandler->AddSignalSemaphore(commandListID, semaphore);
            _commandListHandler->EndCommandList(commandListID, VK_NULL_HANDLE);

            // Reset staging buffer allocators and uploadToBufferTasks
            for (u32 i = 0; i < data->stagingBuffers.Num; i++)
            {
                StagingBuffer& stagingBuffer = data->stagingBuffers.Get(i);

                stagingBuffer.allocator.Reset();
                stagingBuffer.isSubmitted = false;
            }

            data->isDirty = false;
        }

        std::shared_ptr<UploadBuffer> UploadBufferHandlerVK::CreateUploadBuffer(BufferID targetBuffer, size_t targetOffset, size_t size)
        {
            if (size > BUFFER_SIZE)
            {
                DebugHandler::PrintFatal("Requested bigger staging memory than our staging buffer size!");
            }

            UploadBufferHandlerVKData* data = static_cast<UploadBufferHandlerVKData*>(_data);

            void* mappedMemory = nullptr;
            StagingBufferID stagingBufferID;

            size_t offset = Allocate(size, stagingBufferID, mappedMemory);

            UploadToBufferTask task;
            task.targetBuffer = targetBuffer;
            task.targetOffset = targetOffset;
            task.stagingBufferOffset = offset;
            task.copySize = size;

            StagingBuffer& stagingBuffer = data->stagingBuffers.Get(static_cast<StagingBufferID::type>(stagingBufferID));
            stagingBuffer.uploadToBufferTasks.enqueue(task);

            // Increment number of active handles
            {
                std::scoped_lock lock(stagingBuffer.handleLock);
                stagingBuffer.activeHandles++;
            }

            std::shared_ptr<UploadBuffer> uploadBuffer(new UploadBuffer(),
                [&](UploadBuffer* buffer)
                {
                    {
                        std::scoped_lock lock(stagingBuffer.handleLock);
                        // Decrement the number of active handles into this staging buffer
                        if (--stagingBuffer.activeHandles == 0)
                        {
                            // If we are the last active handle and the staging buffer is full, execute it
                            if (stagingBuffer.isFull)
                            {
                                ExecuteStagingBuffer(stagingBuffer);
                                stagingBuffer.isFull = false;
                            }
                        }
                    }
                    delete buffer;
                });

            uploadBuffer->size = size;
            uploadBuffer->mappedMemory = mappedMemory;

            data->isDirty = true;
            return uploadBuffer;
        }

        std::shared_ptr<UploadBuffer> UploadBufferHandlerVK::CreateUploadBuffer(TextureID targetTexture, size_t targetOffset, size_t size)
        {
            if (size > BUFFER_SIZE)
            {
                DebugHandler::PrintFatal("Requested bigger staging memory than our staging buffer size!");
            }

            UploadBufferHandlerVKData* data = static_cast<UploadBufferHandlerVKData*>(_data);

            void* mappedMemory = nullptr;
            StagingBufferID stagingBufferID;
            size_t offset = Allocate(size, stagingBufferID, mappedMemory);

            UploadToTextureTask task;
            task.targetTexture = targetTexture;
            task.targetOffset = targetOffset;
            task.stagingBufferOffset = offset;

            StagingBuffer& stagingBuffer = data->stagingBuffers.Get(static_cast<StagingBufferID::type>(stagingBufferID));
            stagingBuffer.uploadToTextureTasks.enqueue(task);

            // Increment number of active handles
            {
                std::scoped_lock lock(stagingBuffer.handleLock);
                stagingBuffer.activeHandles++;
            }

            std::shared_ptr<UploadBuffer> uploadBuffer(new UploadBuffer(),
                [&](UploadBuffer* buffer)
                {
                    {
                        std::scoped_lock lock(stagingBuffer.handleLock);
                        // Decrement the number of active handles into this staging buffer
                        if (--stagingBuffer.activeHandles == 0)
                        {
                            // If we are the last active handle and the staging buffer is full, execute it
                            if (stagingBuffer.isFull)
                            {
                                ExecuteStagingBuffer(stagingBuffer);
                                stagingBuffer.isFull = false;
                            }
                        }
                    }
                    delete buffer;
                });

            uploadBuffer->size = size;
            uploadBuffer->mappedMemory = mappedMemory;

            data->isDirty = true;
            return uploadBuffer;
        }

        SemaphoreID UploadBufferHandlerVK::GetUploadFinishedSemaphore()
        {
            UploadBufferHandlerVKData* data = static_cast<UploadBufferHandlerVKData*>(_data);
            return data->uploadFinishedSemaphore;
        }

        bool UploadBufferHandlerVK::ShouldWaitForUpload()
        {
            UploadBufferHandlerVKData* data = static_cast<UploadBufferHandlerVKData*>(_data);
            return data->isDirty;
        }

        size_t UploadBufferHandlerVK::Allocate(size_t size, StagingBufferID& stagingBufferID, void*& mappedMemory)
        {
            UploadBufferHandlerVKData* data = static_cast<UploadBufferHandlerVKData*>(_data);
            size_t offset = 0;

            u32 selectedStagingBuffer = data->selectedStagingBuffer;

            StagingBuffer* stagingBuffer = &data->stagingBuffers.Get(selectedStagingBuffer);

            // Try to allocate in the currently selected stagingbuffer
            if (stagingBuffer->allocator.TryAllocateOffset(size, 16, offset))
            {
                stagingBufferID = StagingBufferID(static_cast<StagingBufferID::type>(selectedStagingBuffer));
                mappedMemory = static_cast<void*>(&static_cast<u8*>(stagingBuffer->mappedMemory)[offset]);
                return offset;
            }

            {
                std::scoped_lock lock(data->submitMutex);

                // First thread to reach here should submit it
                if (stagingBuffer->isFirstThreadToReach)
                {
                    stagingBuffer->isFirstThreadToReach = false;

                    // If the buffer is full and nobody is currently writing into it, submit it.
                    // Otherwise it gets submitted when the last writer releases their active handle
                    {
                        std::scoped_lock lock(stagingBuffer->handleLock);
                        stagingBuffer->isFull = true;
                        if (stagingBuffer->activeHandles == 0)
                        {
                            ExecuteStagingBuffer(*stagingBuffer);
                        }
                    }

                    // Get for the next staging buffer
                    selectedStagingBuffer = (selectedStagingBuffer + 1) % data->stagingBuffers.Num;
                    data->selectedStagingBuffer = selectedStagingBuffer;

                    stagingBuffer = &data->stagingBuffers.Get(selectedStagingBuffer);

                    // Wait for it if needed
                    WaitForStagingBuffer(*stagingBuffer);
                    stagingBuffer->isFirstThreadToReach = true;

                    {
                        std::scoped_lock lock(stagingBuffer->handleLock);
                        stagingBuffer->isFull = false;
                    }
                }
                else
                {
                    // Just grab the new stagingbuffer
                    selectedStagingBuffer = data->selectedStagingBuffer;
                    stagingBuffer = &data->stagingBuffers.Get(selectedStagingBuffer);
                }
            }

            // Try to allocate in the newly selected stagingbuffer
            if (stagingBuffer->allocator.TryAllocateOffset(size, 16, offset))
            {
                stagingBufferID = StagingBufferID(static_cast<StagingBufferID::type>(selectedStagingBuffer));
                mappedMemory = static_cast<void*>(&static_cast<u8*>(stagingBuffer->mappedMemory)[offset]);
                return offset;
            }
            
            DebugHandler::PrintFatal("Could not allocate in staging buffer even after getting a second buffer");
            return offset;
        }

        void UploadBufferHandlerVK::ExecuteStagingBuffer(VkCommandBuffer commandBuffer, StagingBuffer& stagingBuffer)
        {
            UploadBufferHandlerVKData* data = static_cast<UploadBufferHandlerVKData*>(_data);

            {
                UploadToBufferTask task;
                while (stagingBuffer.uploadToBufferTasks.try_dequeue(task))
                {
                    VkBuffer dstBuffer = _bufferHandler->GetBuffer(task.targetBuffer);
                    VkBuffer srcBuffer = _bufferHandler->GetBuffer(stagingBuffer.buffer);

                    VkBufferCopy copyRegion = {};
                    copyRegion.dstOffset = task.targetOffset;
                    copyRegion.srcOffset = task.stagingBufferOffset;
                    copyRegion.size = task.copySize;
                    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
                }
            }

            {
                UploadToTextureTask task;
                while (stagingBuffer.uploadToTextureTasks.try_dequeue(task))
                {
                    VkBuffer srcBuffer = _bufferHandler->GetBuffer(stagingBuffer.buffer);

                    _textureHandler->CopyBufferToImage(commandBuffer, srcBuffer, task.stagingBufferOffset, task.targetTexture);
                }
            }

            stagingBuffer.isSubmitted = true;
        }

        void UploadBufferHandlerVK::ExecuteStagingBuffer(StagingBuffer& stagingBuffer)
        {
            // First thread to reach here should submit it
            CommandListID commandListID = _commandListHandler->BeginCommandList(QueueType::Graphics);
            VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

#if TRACY_ENABLE
            TracySourceLocation(RingBufferUpload, "RingBufferUpload", tracy::Color::Yellow2);
            tracy::VkCtxManualScope tracyScope(_device->_tracyContext, &RingBufferUpload, true);
            tracyScope.Start(commandBuffer);
#endif

            ExecuteStagingBuffer(commandBuffer, stagingBuffer);

#if TRACY_ENABLE
            tracyScope.End();
#endif

            _commandListHandler->EndCommandList(commandListID, stagingBuffer.fence);
        }

        void UploadBufferHandlerVK::WaitForStagingBuffer(StagingBuffer& stagingBuffer)
        {
            if (!stagingBuffer.isSubmitted)
                return;

            u64 timeout = 5000000000; // 5 seconds in nanoseconds
            VkResult result = vkWaitForFences(_device->_device, 1, &stagingBuffer.fence, true, timeout);

            if (result == VK_TIMEOUT)
            {
                DebugHandler::PrintFatal("Waiting for staging buffer fence took longer than 5 seconds, something is wrong!");
            }

            vkResetFences(_device->_device, 1, &stagingBuffer.fence);
            stagingBuffer.isSubmitted = false;

            // Reset staging buffer
            stagingBuffer.allocator.Reset();
        }
    }
}
