#pragma once
#include <NovusTypes.h>
#include <vector>
#include <queue>
#include <vulkan/vulkan.h>
#include "../../../FrameResource.h"

#include "../../../Descriptors/CommandListDesc.h"
#include "../../../Descriptors/GraphicsPipelineDesc.h"
#include "../../../Descriptors/ComputePipelineDesc.h"


namespace tracy
{
    class VkCtxManualScope;
}

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        class CommandListHandlerVK
        {
        public:
            void Init(RenderDeviceVK* device);

            void FlipFrame();
            void ResetCommandBuffers();

            CommandListID BeginCommandList();
            void EndCommandList(CommandListID id, VkFence fence);

            VkCommandBuffer GetCommandBuffer(CommandListID id);

            void AddWaitSemaphore(CommandListID id, VkSemaphore semaphore);
            void AddSignalSemaphore(CommandListID id, VkSemaphore semaphore);

            void SetBoundGraphicsPipeline(CommandListID id, GraphicsPipelineID pipelineID);
            void SetBoundComputePipeline(CommandListID id, ComputePipelineID pipelineID);

            GraphicsPipelineID GetBoundGraphicsPipeline(CommandListID id);
            ComputePipelineID GetBoundComputePipeline(CommandListID id);

            tracy::VkCtxManualScope*& GetTracyScope(CommandListID id);

            VkFence GetCurrentFence();

        private:
            struct CommandList
            {
                std::vector<VkSemaphore> waitSemaphores;
                std::vector<VkSemaphore> signalSemaphores;

                VkCommandBuffer commandBuffer;
                VkCommandPool commandPool;

                tracy::VkCtxManualScope* tracyScope = nullptr;

                GraphicsPipelineID boundGraphicsPipeline = GraphicsPipelineID::Invalid();
                ComputePipelineID boundComputePipeline = ComputePipelineID::Invalid();
            };

            CommandListID CreateCommandList();

        private:

        private:
            RenderDeviceVK* _device;

            std::vector<CommandList> _commandLists;
            std::queue<CommandListID> _availableCommandLists;
            
            u8 _frameIndex = 0;
            FrameResource<std::queue<CommandListID>, 2> _closedCommandLists;

            FrameResource<VkFence, 2> _frameFences;
        };
    }
}