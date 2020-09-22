#include "UIDataSingleton.h"
#include <shared_mutex>
#include "../../../../Utils/ServiceLocator.h"
#include "../../../Utils/TransformUtils.h"
#include "../../../angelscript/BaseElement.h"

namespace UISingleton
{
    std::shared_mutex& UIDataSingleton::GetMutex(const entt::entity entId)
    {
        return entityToElement[entId]->_mutex;
    }

    void UIDataSingleton::ClearAllElements()
    {
        std::vector<entt::entity> entityIds;
        entityIds.reserve(entityToElement.size());

        for (auto asObject : entityToElement)
        {
            entityIds.push_back(asObject.first);
            delete asObject.second;
        }
        entityToElement.clear();

        // Delete entities.
        ServiceLocator::GetUIRegistry()->destroy(entityIds.begin(), entityIds.end());

        focusedWidget = entt::null;
    }

    void UIDataSingleton::DestroyElement(entt::entity entId, bool destroyChildren)
    {
        destructionQueue.enqueue(entId);

        if (destroyChildren)
        {
            UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(entId);
            for (UI::UIChild child : transform->children)
            {
                DestroyElement(child.entId, true);
            }
        }
    }
}
