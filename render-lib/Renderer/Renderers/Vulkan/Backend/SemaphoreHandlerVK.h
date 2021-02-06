#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan_core.h>

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

        struct ISemaphoreHandlerVKData {};

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

            ISemaphoreHandlerVKData* _data;
        };
    }
}