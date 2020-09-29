#include "UIDataSingleton.h"
#include "../../../../Utils/ServiceLocator.h"
#include "../../../angelscript/BaseElement.h"

namespace UISingleton
{
    void UIDataSingleton::ClearAllElements()
    {
        std::vector<entt::entity> entityIds;
        entityIds.reserve(entityToElement.size());

        for (auto pair : entityToElement)
        {
            entityIds.push_back(pair.first);
            delete pair.second;
        }
        entityToElement.clear();

        // Delete entities.
        ServiceLocator::GetUIRegistry()->destroy(entityIds.begin(), entityIds.end());

        focusedWidget = entt::null;
    }
}
