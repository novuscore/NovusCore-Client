#include "UIDataSingleton.h"
#include <shared_mutex>
#include "../../../../Utils/ServiceLocator.h"
#include "../../../Utils/TransformUtils.h"
#include "../../../angelscript/BaseElement.h"

namespace UISingleton
{
    std::shared_mutex& UIDataSingleton::GetMutex(entt::entity entId)
    {
        return entityToAsObject[entId]->_mutex;
    }

    void UIDataSingleton::ClearWidgets()
    {
        std::vector<entt::entity> entityIds;
        entityIds.reserve(entityToAsObject.size());

        for (auto asObject : entityToAsObject)
        {
            entityIds.push_back(asObject.first);
            delete asObject.second;
        }
        entityToAsObject.clear();

        // Delete entities.
        ServiceLocator::GetUIRegistry()->destroy(entityIds.begin(), entityIds.end());

        focusedWidget = entt::null;
    }

    void UIDataSingleton::DestroyWidget(entt::entity entId)
    {
        destructionQueue.enqueue(entId);
    }
}
