#include "SemaphoreHandlerVK.h"

#include <cassert>
#include <vector>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include <Utils/DebugHandler.h>
#include <vulkan/vulkan.h>

#include "RenderDeviceVK.h"

namespace Renderer
{
    namespace Backend
    {
        struct SemaphoreHandlerVKData : ISemaphoreHandlerVKData
        {
            std::vector<VkSemaphore> semaphores;
        };

        void SemaphoreHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;
            _data = new SemaphoreHandlerVKData();
        }

        SemaphoreID SemaphoreHandlerVK::CreateNSemaphore()
        {
            SemaphoreHandlerVKData& data = static_cast<SemaphoreHandlerVKData&>(*_data);

            size_t nextID = data.semaphores.size();
            // Make sure we haven't exceeded the limit of the SemaphoreID type, if this hits you need to change type of SemaphoreID to something bigger
            assert(nextID < SemaphoreID::MaxValue());

            VkSemaphore& semaphore = data.semaphores.emplace_back();

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            if (vkCreateSemaphore(_device->_device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to create sampler!");
            }

            return SemaphoreID(static_cast<SemaphoreID::type>(nextID));
        }

        VkSemaphore SemaphoreHandlerVK::GetVkSemaphore(SemaphoreID id)
        {
            SemaphoreHandlerVKData& data = static_cast<SemaphoreHandlerVKData&>(*_data);

            // Lets make sure this id exists
            assert(data.semaphores.size() > static_cast<SemaphoreID::type>(id));

            return data.semaphores[static_cast<SemaphoreID::type>(id)];
        }
    }
}