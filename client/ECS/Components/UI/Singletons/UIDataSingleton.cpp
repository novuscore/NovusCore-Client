#include "UIDataSingleton.h"

#include "../../../../Utils/ServiceLocator.h"

#include "../../../../Scripting/Classes/UI/asUITransform.h"

namespace UI
{
    void UIDataSingleton::ClearWidgets()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        std::vector<entt::entity> entityIds = std::vector<entt::entity>();
        entityIds.reserve(entityToAsObject.size());

        for (auto asObject : entityToAsObject)
        {
            entityIds.push_back(asObject.first);

            delete asObject.second;
        }
        entityToAsObject.clear();

        // Delete entities.
        registry->destroy(entityIds.begin(), entityIds.end());

        focusedWidget = entt::null;
    }

    void UIDataSingleton::DestroyWidget(entt::entity entId)
    {
        if (auto itr = entityToAsObject.find(entId); itr != entityToAsObject.end())
        {
            delete itr->second;
            entityToAsObject.erase(entId);
        }

        ServiceLocator::GetUIRegistry()->destroy(entId);
    }
}
