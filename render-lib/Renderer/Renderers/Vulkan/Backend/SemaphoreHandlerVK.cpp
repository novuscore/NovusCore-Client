#include "SemaphoreHandlerVK.h"
#include <Utils/DebugHandler.h>
#include <cassert>
#include "RenderDeviceVK.h"
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

namespace Renderer
{
    namespace Backend
    {
        void SemaphoreHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;
        }

        GPUSemaphoreID SemaphoreHandlerVK::CreateGPUSemaphore()
        {
            using type = type_safe::underlying_type<GPUSemaphoreID>;

            size_t nextID = _semaphores.size();
            // Make sure we haven't exceeded the limit of the SemaphoreID type, if this hits you need to change type of SemaphoreID to something bigger
            assert(nextID < GPUSemaphoreID::MaxValue());

            VkSemaphore& semaphore = _semaphores.emplace_back();

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            if (vkCreateSemaphore(_device->_device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create sampler!");
            }

            return GPUSemaphoreID(static_cast<type>(nextID));
        }

        VkSemaphore SemaphoreHandlerVK::GetVkSemaphore(GPUSemaphoreID id)
        {
            using type = type_safe::underlying_type<GPUSemaphoreID>;

            // Lets make sure this id exists
            assert(_semaphores.size() > static_cast<type>(id));

            return _semaphores[static_cast<type>(id)];
        }
    }
}