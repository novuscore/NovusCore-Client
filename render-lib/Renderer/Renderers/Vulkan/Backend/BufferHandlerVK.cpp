#include "BufferHandlerVK.h"
#include "RenderDeviceVK.h"


namespace Renderer
{
    namespace Backend
    {
        BufferHandlerVK::BufferHandlerVK()
        {
        }

        BufferHandlerVK::~BufferHandlerVK()
        {
        }

        void BufferHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;
        }

        BufferID BufferHandlerVK::CreateBuffer(BufferDesc& desc)
        {

        }
    }
}
