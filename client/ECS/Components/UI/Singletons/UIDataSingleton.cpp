#include "UIDataSingleton.h"
#include "../../../../Utils/ServiceLocator.h"
#include "../../../../UI/TransformUtils.h"
#include "../../../../Scripting/Classes/UI/asUITransform.h"

namespace UI
{
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
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UITransform& transform = registry->get<UITransform>(entId);
        for (UIChild& child : transform.children)
        {
            UI::TransformUtils::RemoveParent(registry->get<UITransform>(entt::entity(child.entity)));

            if (auto itr = entityToAsObject.find(entId); itr != entityToAsObject.end())
                UI::TransformUtils::RemoveParent(itr->second->_transform);
        }

        if (auto itr = entityToAsObject.find(entId); itr != entityToAsObject.end())
        {
            delete itr->second;
            entityToAsObject.erase(entId);
        }

        registry->destroy(entId);
    }
}
