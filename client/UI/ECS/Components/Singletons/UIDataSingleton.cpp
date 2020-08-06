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
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(entId);
        for (UI::UIChild& child : transform->children)
        {
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);

            childTransform->position = childTransform->position + childTransform->localPosition;
            childTransform->localPosition = vec2(0, 0);
            childTransform->parent = entt::null;
        }

        auto itr = entityToAsObject.find(entId);
        if (itr != entityToAsObject.end())
        {
            delete itr->second;
            entityToAsObject.erase(itr);
        }

        registry->destroy(entId);
    }
}
