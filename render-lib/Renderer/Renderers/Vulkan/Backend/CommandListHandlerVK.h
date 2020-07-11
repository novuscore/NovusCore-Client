#pragma once
#include <NovusTypes.h>
#include <vector>
#include <queue>
#include <vulkan/vulkan.h>

#include "../../../Descriptors/CommandListDesc.h"
#include "../../../Descriptors/GraphicsPipelineDesc.h"


namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        class CommandListHandlerVK
        {
        public:
            void Init(RenderDeviceVK* device);

            CommandListID BeginCommandList();
            void EndCommandList(CommandListID id);

            VkCommandBuffer GetCommandBuffer(CommandListID id);

            bool GetWaitSemaphore(CommandListID id, VkSemaphore& semaphore);
            void SetWaitSemaphore(CommandListID id, VkSemaphore semaphore);

            bool GetSignalSemaphore(CommandListID id, VkSemaphore& semaphore);
            void SetSignalSemaphore(CommandListID id, VkSemaphore semaphore);

            void SetBoundGraphicsPipeline(CommandListID id, GraphicsPipelineID pipelineID);
            GraphicsPipelineID GetBoundGraphicsPipeline(CommandListID id);

        private:
            struct CommandList
            {
                VkSemaphore waitSemaphore = NULL;
                VkSemaphore signalSemaphore = NULL;
                VkCommandBuffer commandBuffer;
                VkCommandPool commandPool;

                GraphicsPipelineID boundGraphicsPipeline = GraphicsPipelineID::Invalid();
            };

            CommandListID CreateCommandList();

        private:

        private:
            RenderDeviceVK* _device;

            std::vector<CommandList> _commandLists;
            std::queue<CommandListID> _availableCommandLists;
        };
    }
}