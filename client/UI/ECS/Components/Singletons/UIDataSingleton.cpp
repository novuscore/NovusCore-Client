#include "UIDataSingleton.h"
#include "../../../../Utils/ServiceLocator.h"
#include "../../../Utils/TransformUtils.h"
#include "../../../angelscript/BaseElement.h"

namespace UISingleton
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
        UIComponent::Transform& transform = registry->get<UIComponent::Transform>(entId);
        for (UI::UIChild& child : transform.children)
        {
            UIUtils::Transform::RemoveParent(registry->get<UIComponent::Transform>(entt::entity(child.entity)));

            if (auto itr = entityToAsObject.find(entId); itr != entityToAsObject.end())
                UIUtils::Transform::RemoveParent(itr->second->_transform);
        }

        if (auto itr = entityToAsObject.find(entId); itr != entityToAsObject.end())
        {
            delete itr->second;
            entityToAsObject.erase(entId);
        }

        registry->destroy(entId);
    }
}
