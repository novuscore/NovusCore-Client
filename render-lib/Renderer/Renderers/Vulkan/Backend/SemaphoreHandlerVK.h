#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan_core.h>

#include "../../../Descriptors/SemaphoreDesc.h"

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

            SemaphoreID CreateNSemaphore();

            VkSemaphore GetVkSemaphore(SemaphoreID id);

        private:
            

        private:

        private:
            RenderDeviceVK* _device;

            ISemaphoreHandlerVKData* _data;
        };
    }
}