#pragma once
#include <NovusTypes.h>

#include "../../../Descriptors/BufferDesc.h"

#include "vk_mem_alloc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        struct BufferHandlerVK
        {
            BufferHandlerVK();
            ~BufferHandlerVK();

            void Init(RenderDeviceVK* device);

            BufferID CreateBuffer(BufferDesc& desc);

        private:
            RenderDeviceVK* _device;

            friend class RendererVK;
        };
    }
}