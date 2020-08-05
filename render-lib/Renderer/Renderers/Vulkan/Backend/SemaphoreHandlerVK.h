#pragma once
#include <NovusTypes.h>
#include <vector>
#include <vulkan/vulkan.h>

#include "../../../Descriptors/GPUSemaphoreDesc.h"


namespace tracy
{
    class VkCtxManualScope;
}

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        class SemaphoreHandlerVK
        {
        public:
            void Init(RenderDeviceVK* device);

            GPUSemaphoreID CreateGPUSemaphore();

            VkSemaphore GetVkSemaphore(GPUSemaphoreID id);

        private:
            

        private:

        private:
            RenderDeviceVK* _device;

            std::vector<VkSemaphore> _semaphores;
        };
    }
}