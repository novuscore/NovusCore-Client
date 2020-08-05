#include "CommandListHandlerVK.h"
#include <Utils/DebugHandler.h>
#include <cassert>
#include "RenderDeviceVK.h"
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

namespace Renderer
{
    namespace Backend
    {
        void CommandListHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (u32 i = 0; i < _frameFences.Num; i++)
            {
                vkCreateFence(_device->_device, &fenceInfo, nullptr, &_frameFences.Get(i));
            }
        }

        void CommandListHandlerVK::FlipFrame()
        {
            _frameIndex++;

            if (_frameIndex >= _closedCommandLists.Num)
            {
                _frameIndex = 0;
            }
        }

        void CommandListHandlerVK::ResetCommandBuffers()
        {
            using type = type_safe::underlying_type<CommandListID>;
            std::queue<CommandListID>& closedCommandLists = _closedCommandLists.Get(_frameIndex);

            while (!closedCommandLists.empty())
            {
                CommandListID commandListID = closedCommandLists.front();
                closedCommandLists.pop();

                CommandList& commandList = _commandLists[static_cast<type>(commandListID)];

                // Reset commandlist
                vkResetCommandPool(_device->_device, commandList.commandPool, 0);

                // Push the commandlist into availableCommandLists
                _availableCommandLists.push(commandListID);
            }
        }

        CommandListID CommandListHandlerVK::BeginCommandList()
        {
            using type = type_safe::underlying_type<CommandListID>;

            CommandListID id;
            if (!_availableCommandLists.empty())
            {
                id = _availableCommandLists.front();
                _availableCommandLists.pop();

                CommandList& commandList = _commandLists[static_cast<type>(id)];

                VkCommandBufferBeginInfo beginInfo = {};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = 0; // Optional
                beginInfo.pInheritanceInfo = nullptr; // Optional

                if (vkBeginCommandBuffer(commandList.commandBuffer, &beginInfo) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to begin recording command buffer!");
                }
            }
            else
            {
                return CreateCommandList();
            }

            return id;
        }

        void CommandListHandlerVK::EndCommandList(CommandListID id, VkFence fence)
        {
            ZoneScopedC(tracy::Color::Red3)

            using type = type_safe::underlying_type<CommandListID>;
            CommandList& commandList = _commandLists[static_cast<type>(id)];

            {
                ZoneScopedNC("Submit", tracy::Color::Red3)

                // Close command list
                if (vkEndCommandBuffer(commandList.commandBuffer) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to record command buffer!");
                }

                // Execute command list
                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandList.commandBuffer;

                u32 numWaitSemaphores = static_cast<u32>(commandList.waitSemaphores.size());
                std::vector<VkPipelineStageFlags> dstStageMasks(numWaitSemaphores);

                for (VkPipelineStageFlags& dstStageMask : dstStageMasks)
                {
                    dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }

                submitInfo.waitSemaphoreCount = numWaitSemaphores;
                submitInfo.pWaitSemaphores = commandList.waitSemaphores.data();
                submitInfo.pWaitDstStageMask = dstStageMasks.data();
                
                submitInfo.signalSemaphoreCount = static_cast<u32>(commandList.signalSemaphores.size());
                submitInfo.pSignalSemaphores = commandList.signalSemaphores.data();

                vkQueueSubmit(_device->_graphicsQueue, 1, &submitInfo, fence);
            }

            commandList.waitSemaphores.clear();
            commandList.signalSemaphores.clear();
            commandList.boundGraphicsPipeline = GraphicsPipelineID::Invalid();

            _closedCommandLists.Get(_frameIndex).push(id);
        }

        VkCommandBuffer CommandListHandlerVK::GetCommandBuffer(CommandListID id)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            CommandList& commandList = _commandLists[static_cast<type>(id)];

            return commandList.commandBuffer;
        }

        void CommandListHandlerVK::AddWaitSemaphore(CommandListID id, VkSemaphore semaphore)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            CommandList& commandList = _commandLists[static_cast<type>(id)];

            commandList.waitSemaphores.push_back(semaphore);
        }

        void CommandListHandlerVK::AddSignalSemaphore(CommandListID id, VkSemaphore semaphore)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            CommandList& commandList = _commandLists[static_cast<type>(id)];

            commandList.signalSemaphores.push_back(semaphore);
        }

        void CommandListHandlerVK::SetBoundGraphicsPipeline(CommandListID id, GraphicsPipelineID pipelineID)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            CommandList& commandList = _commandLists[static_cast<type>(id)];

            commandList.boundGraphicsPipeline = pipelineID;
        }

        GraphicsPipelineID CommandListHandlerVK::GetBoundGraphicsPipeline(CommandListID id)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            return _commandLists[static_cast<type>(id)].boundGraphicsPipeline;
        }

        tracy::VkCtxManualScope*& CommandListHandlerVK::GetTracyScope(CommandListID id)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            return _commandLists[static_cast<type>(id)].tracyScope;
        }

        VkFence CommandListHandlerVK::GetCurrentFence()
        {
            return _frameFences.Get(_frameIndex);
        }

        CommandListID CommandListHandlerVK::CreateCommandList()
        {
            size_t id = _commandLists.size();
            assert(id < CommandListID::MaxValue());
            using type = type_safe::underlying_type<CommandListID>;

            CommandList commandList;

            // Create commandpool
            QueueFamilyIndices queueFamilyIndices = _device->FindQueueFamilies(_device->_physicalDevice);

            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if (vkCreateCommandPool(_device->_device, &poolInfo, nullptr, &commandList.commandPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create command pool!");
            }

            // Create commandlist
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandList.commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(_device->_device, &allocInfo, &commandList.commandBuffer) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to allocate command buffers!");
            }

            // Open commandlist
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(commandList.commandBuffer, &beginInfo) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to begin recording command buffer!");
            }

            _commandLists.push_back(commandList);

            return CommandListID(static_cast<type>(id));
        }
    }
}