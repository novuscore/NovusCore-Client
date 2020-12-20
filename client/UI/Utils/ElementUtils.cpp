#include "ElementUtils.h"
#include <entity/registry.hpp>
#include "../../Utils/ServiceLocator.h"
#include "../angelscript/BaseElement.h"

#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "../ECS/Components/Relation.h"
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Destroy.h"
#include "../ECS/Components/Dirty.h"

#include "TransformUtils.h"

namespace UIUtils
{
    void ClearAllElements()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton* dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();

        std::vector<entt::entity> entityIds;
        entityIds.reserve(dataSingleton->entityToElement.size());

        for (auto& pair : dataSingleton->entityToElement)
        {
            entityIds.push_back(pair.first);
            delete pair.second;
        }
        dataSingleton->entityToElement.clear();

        // Delete entities.
        registry->destroy(entityIds.begin(), entityIds.end());

        dataSingleton->focusedElement = entt::null;
        dataSingleton->hoveredElement = entt::null;
        dataSingleton->draggedElement = entt::null;
    }

    void MarkChildrenDirty(entt::registry* registry, const entt::entity entityId)
    {
        const UIComponent::Relation* relation = &registry->get<UIComponent::Relation>(entityId);
        for (const UI::UIChild& child : relation->children)
        {
            if (!registry->has<UIComponent::Dirty>(child.entId))
                registry->emplace<UIComponent::Dirty>(child.entId);

            MarkChildrenDirty(registry, child.entId);
        }
    }

    void MarkChildrenForDestruction(entt::registry* registry, entt::entity entityId)
    {
        const UIComponent::Relation* relation = &registry->get<UIComponent::Relation>(entityId);
        for (const UI::UIChild& child : relation->children)
        {
            if (!registry->has<UIComponent::Destroy>(child.entId))
                registry->emplace<UIComponent::Destroy>(child.entId);

            MarkChildrenForDestruction(registry, entityId);
        }
    }
    
    void RemoveFromParent(entt::registry* registry, entt::entity child)
    {
        auto[childRelation,childTransform] = registry->get<UIComponent::Relation, UIComponent::Transform>(child);
        UIComponent::Relation& parentRelation = registry->get<UIComponent::Relation>(childRelation.parent);

        childRelation.parent = entt::null;
        childTransform.anchorPosition = UIUtils::Transform::GetAnchorPositionOnScreen(childTransform.anchor);

        parentRelation.children.erase(std::remove_if(parentRelation.children.begin(), parentRelation.children.end(), [child](UI::UIChild& uiChild) { return uiChild.entId == child; }), parentRelation.children.end());
    }
}