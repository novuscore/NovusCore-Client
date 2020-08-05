#include "UIEntityPoolSingleton.h"
#include "../../../../Utils/ServiceLocator.h"

namespace UISingleton
{
    void UIEntityPoolSingleton::AllocatePool()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        std::vector<entt::entity> entityIds(ENTITIES_TO_PREALLOCATE);
        registry->create(entityIds.begin(), entityIds.end());

        std::sort(entityIds.begin(), entityIds.end(), [](entt::entity first, entt::entity second) { return first < second; });

        entityIdPool.enqueue_bulk(entityIds.begin(), ENTITIES_TO_PREALLOCATE);
    }

    entt::entity UIEntityPoolSingleton::GetId()
    {
        entt::entity entityId;

        if (!entityIdPool.try_dequeue(entityId))
        {
            AllocatePool();
            entityIdPool.try_dequeue(entityId);
        }

        return entityId;
    }
}
