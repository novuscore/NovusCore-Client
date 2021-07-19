#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan_core.h>

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

        struct ICommandListHandlerVKData {};

        enum QueueType
        {
            Graphics,
            Transfer,
            COUNT
        };

        class CommandListHandlerVK
        {
        public:
            void Init(RenderDeviceVK* device);

            void FlipFrame();
            void ResetCommandBuffers();

            CommandListID BeginCommandList(QueueType queueType);
            void EndCommandList(CommandListID id, VkFence fence);

            VkCommandBuffer GetCommandBuffer(CommandListID id);

            void AddWaitSemaphore(CommandListID id, VkSemaphore semaphore);
            void AddSignalSemaphore(CommandListID id, VkSemaphore semaphore);

            void SetBoundGraphicsPipeline(CommandListID id, GraphicsPipelineID pipelineID);
            void SetBoundComputePipeline(CommandListID id, ComputePipelineID pipelineID);

            GraphicsPipelineID GetBoundGraphicsPipeline(CommandListID id);
            ComputePipelineID GetBoundComputePipeline(CommandListID id);

            i8 GetRenderPassOpenCount(CommandListID id);
            void SetRenderPassOpenCount(CommandListID id, i8 count);

            tracy::VkCtxManualScope*& GetTracyScope(CommandListID id);

            VkFence GetCurrentFence();

        private:
            CommandListID CreateCommandList(QueueType queueType);

        private:

        private:
            RenderDeviceVK* _device;

            ICommandListHandlerVKData* _data;
        };
    }
}