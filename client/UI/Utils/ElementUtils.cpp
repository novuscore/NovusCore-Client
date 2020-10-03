#include "ElementUtils.h"
#include "../../Utils/ServiceLocator.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Destroy.h"
#include "../angelscript/BaseElement.h"
#include <entity/registry.hpp>
#include <tracy/Tracy.hpp>

namespace UIUtils
{
    void ClearAllElements()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton* dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();

        std::vector<entt::entity> entityIds;
        entityIds.reserve(dataSingleton->entityToElement.size());

        for (auto pair : dataSingleton->entityToElement)
        {
            entityIds.push_back(pair.first);
            delete pair.second;
        }
        dataSingleton->entityToElement.clear();

        // Delete entities.
        registry->destroy(entityIds.begin(), entityIds.end());

        dataSingleton->focusedWidget = entt::null;
        dataSingleton->hoveredWidget = entt::null;
        dataSingleton->draggedWidget = entt::null;
    }

    void MarkChildrenForDestruction(entt::registry* registry, entt::entity entityId)
    {
        const UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(entityId);
        for (const UI::UIChild& child : transform->children)
        {
            if (!registry->has<UIComponent::Destroy>(child.entId))
                registry->emplace<UIComponent::Destroy>(child.entId);

            MarkChildrenForDestruction(registry, entityId);
        }
    }
}