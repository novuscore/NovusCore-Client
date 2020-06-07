#include "CommandListHandlerVK.h"
#include <Utils/DebugHandler.h>
#include <cassert>
#include "RenderDeviceVK.h"

namespace Renderer
{
    namespace Backend
    {
        CommandListHandlerVK::CommandListHandlerVK()
        {

        }

        CommandListHandlerVK::~CommandListHandlerVK()
        {
            for (auto& commandList : _commandLists)
            {
                // TODO: Cleanup
            }
            _commandLists.clear();
        }

        CommandListID CommandListHandlerVK::BeginCommandList(RenderDeviceVK* device)
        {
            using type = type_safe::underlying_type<CommandListID>;

            CommandListID id;
            if (_availableCommandLists.size() > 0)
            {
                id = _availableCommandLists.front();
                _availableCommandLists.pop();

                CommandList& commandList = _commandLists[static_cast<type>(id)];

                // Reset commandlist
                vkResetCommandPool(device->_device, commandList.commandPool, 0);
                //vkResetCommandBuffer(commandList.commandBuffer, 0); // Not needed?;

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
                return CreateCommandList(device);
            }

            return id;
        }

        void CommandListHandlerVK::EndCommandList(RenderDeviceVK* device, CommandListID id)
        {
            using type = type_safe::underlying_type<CommandListID>;
            CommandList& commandList = _commandLists[static_cast<type>(id)];

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

            if (commandList.waitSemaphore != NULL)
            {
                VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = &commandList.waitSemaphore;
                submitInfo.pWaitDstStageMask = &dstStageMask;
            }

            if (commandList.signalSemaphore != NULL)
            {
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &commandList.signalSemaphore;
            }

            VkFence fence;
            VkFenceCreateInfo create_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

            vkCreateFence(device->_device, &create_info, nullptr, &fence);

            vkQueueSubmit(device->_graphicsQueue, 1, &submitInfo, fence);
            
            // Do syncing stuff
            vkWaitForFences(device->_device, 1, &fence, VK_TRUE, 1000000);

            vkDestroyFence(device->_device, fence, nullptr);

            commandList.waitSemaphore = NULL;
            commandList.signalSemaphore = NULL;
            commandList.boundGraphicsPipeline = GraphicsPipelineID::Invalid();

            _availableCommandLists.push(id);
        }

        VkCommandBuffer CommandListHandlerVK::GetCommandBuffer(CommandListID id)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            CommandList& commandList = _commandLists[static_cast<type>(id)];

            return commandList.commandBuffer;
        }

        bool CommandListHandlerVK::GetWaitSemaphore(CommandListID id, VkSemaphore& semaphore)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            CommandList& commandList = _commandLists[static_cast<type>(id)];

            if (commandList.waitSemaphore == NULL)
                return false;

            semaphore = commandList.waitSemaphore;
            return true;
        }

        void CommandListHandlerVK::SetWaitSemaphore(CommandListID id, VkSemaphore semaphore)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            CommandList& commandList = _commandLists[static_cast<type>(id)];

            commandList.waitSemaphore = semaphore;
        }

        bool CommandListHandlerVK::GetSignalSemaphore(CommandListID id, VkSemaphore& semaphore)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            CommandList& commandList = _commandLists[static_cast<type>(id)];

            if (commandList.signalSemaphore == NULL)
                return false;

            semaphore = commandList.signalSemaphore;
            return true;
        }

        void CommandListHandlerVK::SetSignalSemaphore(CommandListID id, VkSemaphore semaphore)
        {
            using type = type_safe::underlying_type<CommandListID>;

            // Lets make sure this id exists
            assert(_commandLists.size() > static_cast<type>(id));

            CommandList& commandList = _commandLists[static_cast<type>(id)];

            commandList.signalSemaphore = semaphore;
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

        CommandListID CommandListHandlerVK::CreateCommandList(RenderDeviceVK* device)
        {
            size_t id = _commandLists.size();
            assert(id < CommandListID::MaxValue());
            using type = type_safe::underlying_type<CommandListID>;

            CommandList commandList;

            // Create commandpool
            QueueFamilyIndices queueFamilyIndices = device->FindQueueFamilies(device->_physicalDevice);

            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if (vkCreateCommandPool(device->_device, &poolInfo, nullptr, &commandList.commandPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create command pool!");
            }

            // Create commandlist
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandList.commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(device->_device, &allocInfo, &commandList.commandBuffer) != VK_SUCCESS)
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