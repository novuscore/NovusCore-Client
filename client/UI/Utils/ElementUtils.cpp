#include "ElementUtils.h"
#include <entity/registry.hpp>
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "../ECS/Components/ElementInfo.h"
#include "../ECS/Components/Root.h"
#include "../ECS/Components/Relation.h"
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Destroy.h"
#include "../ECS/Components/Dirty.h"
#include "../ECS/Components/SortKeyDirty.h"

#include "TransformUtils.h"
#include "SortUtils.h"

namespace UIUtils
{
    void ClearAllElements()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        auto deleteView = registry->view<UIComponent::ElementInfo>();
        deleteView.each([&](UIComponent::ElementInfo& elementInfo)
        {
            delete elementInfo.scriptingObject;
        });
        registry->destroy(deleteView.begin(), deleteView.end());

        // Delete entities.
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();
        dataSingleton.entityToElement.clear();

        dataSingleton.focusedElement = entt::null;
        dataSingleton.hoveredElement = entt::null;
        dataSingleton.pressedElement = entt::null;

        dataSingleton.resizedElement = entt::null;
        dataSingleton.draggedElement = entt::null;
    }

    void MarkChildrenDirty(entt::registry* registry, const entt::entity entityId)
    {
        const UIComponent::Relation* relation = &registry->get<UIComponent::Relation>(entityId);
        for (const entt::entity childId : relation->children)
        {
            if (!registry->has<UIComponent::Dirty>(childId))
                registry->emplace<UIComponent::Dirty>(childId);

            MarkChildrenDirty(registry, childId);
        }
    }

    void MarkChildrenForDestruction(entt::registry* registry, entt::entity entityId)
    {
        const UIComponent::Relation* relation = &registry->get<UIComponent::Relation>(entityId);
        for (const entt::entity childId : relation->children)
        {
            if (!registry->has<UIComponent::Destroy>(childId))
                registry->emplace<UIComponent::Destroy>(childId);

            MarkChildrenForDestruction(registry, entityId);
        }
    }
    
    void AddChild(entt::registry* registry, entt::entity parent, entt::entity child)
    {
        auto [childRelation, childTransform] = registry->get<UIComponent::Relation, UIComponent::Transform>(child);
        auto [parentRelation, parentTransform] = registry->get<UIComponent::Relation, UIComponent::Transform>(parent);

        childRelation.parent = parent;
        childTransform.anchorPosition = UIUtils::Transform::GetAnchorPositionInElement(parentTransform, childTransform.anchor);
        if (childTransform.HasFlag(UI::TransformFlags::FILL_PARENTSIZE))
            childTransform.size = UIUtils::Transform::GetInnerSize(&parentTransform);

        registry->remove<UIComponent::Root>(child);
        UIUtils::Transform::UpdateChildTransforms(registry, child);

        parentRelation.children.push_back(child);
        UIUtils::Sort::MarkSortTreeDirty(registry, parent);
    }
    void RemoveFromParent(entt::registry* registry, entt::entity child)
    {
        auto[childRelation,childTransform] = registry->get<UIComponent::Relation, UIComponent::Transform>(child);
        UIComponent::Relation& parentRelation = registry->get<UIComponent::Relation>(childRelation.parent);

        childRelation.parent = entt::null;
        childTransform.anchorPosition = UIUtils::Transform::GetAnchorPositionOnScreen(childTransform.anchor);

        parentRelation.children.erase(std::remove(parentRelation.children.begin(), parentRelation.children.end(), child), parentRelation.children.end());

        registry->emplace<UIComponent::Root>(child);
        if (!registry->has<UIComponent::SortKeyDirty>(child))
            registry->emplace<UIComponent::SortKeyDirty>(child);
    }
}