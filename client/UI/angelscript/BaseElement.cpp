#include "BaseElement.h"
#include <tracy/Tracy.hpp>
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "../ECS/Components/Singletons/UIAddElementQueueSingleton.h"
#include "../ECS/Components/Singletons/UIEntityPoolSingleton.h"

#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Collidable.h"
#include "../ECS/Components/Dirty.h"
#include "../Utils/TransformUtils.h"
#include "../Utils/VisibilityUtils.h"

namespace UIScripting
{
    BaseElement::BaseElement(UI::UIElementType elementType) : _elementType(elementType)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        _entityId = registry->ctx<UISingleton::UIEntityPoolSingleton>().GetId();
        registry->ctx<UISingleton::UIDataSingleton>().entityToAsObject[_entityId] = this;
    }

    void BaseElement::SetTransform(const vec2& position, const vec2& size)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);

        const bool hasParent = transform->parent != entt::null;
        if (hasParent)
            transform->localPosition = position;
        else
            transform->position = position;

        // Don't change size if we are trying to fill parent size since it will just adjust to the parent.
        if (!transform->fillParentSize)
            transform->size = size;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
        MarkDirty(registry, _entityId);
    }

    const vec2 BaseElement::GetScreenPosition() const
    {
        UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return UIUtils::Transform::GetScreenPosition(transform);
    }
    const vec2 BaseElement::GetLocalPosition() const
    {
        UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->parent == entt::null ? vec2(0, 0) : transform->localPosition;
    }
    const vec2 BaseElement::GetParentPosition() const
    {
        UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->parent == entt::null ? vec2(0, 0) : transform->position;
    }
    void BaseElement::SetPosition(const vec2& position)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);

        const bool hasParent = transform->parent != entt::null;
        if (hasParent)
            transform->localPosition = position;
        else
            transform->position = position;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
        MarkDirty(registry, _entityId);
    }

    const vec2 BaseElement::GetAnchor() const
    {
        UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->anchor;
    }
    void BaseElement::SetAnchor(const vec2& anchor)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);

        transform->anchor = anchor;

        if (transform->parent == entt::null)
            return;

        UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(transform->parent);
        transform->position = UIUtils::Transform::GetAnchorPosition(parentTransform, transform->anchor);

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
        MarkDirty(registry, _entityId);
    }

    const vec2 BaseElement::GetLocalAnchor() const
    {
        UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->localAnchor;
    }
    void BaseElement::SetLocalAnchor(const vec2& localAnchor)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);

        transform->localAnchor = localAnchor;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
        MarkDirty(registry, _entityId);
    }

    const vec2 BaseElement::GetSize() const
    {
        UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->size;
    }
    void BaseElement::SetSize(const vec2& size)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);

        transform->size = size;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
        MarkDirty(registry, _entityId);
    }

    const bool BaseElement::GetFillParentSize()
    {
        UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->fillParentSize;
    }
    void BaseElement::SetFillParentSize(bool fillParent)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);

        //Check so we are actually changing the state.
        if (fillParent == transform->fillParentSize)
            return;

        transform->fillParentSize = fillParent;

        if (transform->parent != entt::null && fillParent)
        {
            UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(transform->parent);
            transform->size = parentTransform->size;
            UIUtils::Transform::UpdateChildTransforms(registry, transform);
            MarkDirty(registry, _entityId);
        }
    }

    const UI::DepthLayer BaseElement::GetDepthLayer() const
    {
        const UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->sortData.depthLayer;
    }

    void BaseElement::SetDepthLayer(const UI::DepthLayer layer)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);
        transform->sortData.depthLayer = layer;
    }

    const u16 BaseElement::GetDepth() const
    {
        UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);

        return transform->sortData.depth;
    }
    void BaseElement::SetDepth(const u16 depth)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);
        transform->sortData.depth = depth;
    }

    void BaseElement::SetParent(BaseElement* parent)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);
        UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(parent->GetEntityId());

        // Remove old parent.
        if (transform->parent != entt::null)
        {
            auto itr = std::find_if(transform->children.begin(), transform->children.end(), [this](UI::UIChild& uiChild) { return uiChild.entId == _entityId; });
            if (itr != transform->children.end())
                transform->children.erase(itr);
        }

        // Set new parent.
        transform->parent = parent->GetEntityId();

        // Add ourselves to parent's children
        UIUtils::Transform::AddChild(parentTransform, transform);

        // Recalculate new local position whilst keeping screen position.
        vec2 NewOrigin = UIUtils::Transform::GetAnchorPosition(parentTransform, transform->anchor);
        transform->localPosition = transform->position - NewOrigin;
        transform->position = NewOrigin;

        if (transform->fillParentSize)
            transform->size = parentTransform->size;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);

        // Update visibility
        UIComponent::Visibility* visibility = &registry->get<UIComponent::Visibility>(_entityId);
        if (UIUtils::Visibility::UpdateParentVisibility(visibility, registry->has<UIComponent::Visible>(parent->_entityId)))
        {
            const bool newVisibility = visibility->visible && visibility->parentVisible;
            UIUtils::Visibility::UpdateChildVisibility(registry, transform, newVisibility);

            if (newVisibility)
                registry->emplace<UIComponent::Visible>(_entityId);
            else
                registry->remove<UIComponent::Visible>(_entityId);
        }

        MarkDirty(registry, _entityId);
    }

    void BaseElement::UnsetParent()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->parent == entt::null)
            return;
        
        UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(transform->parent);

        UIUtils::Transform::RemoveChild(parentTransform, transform);
    }

    const bool BaseElement::GetExpandBoundsToChildren()
    {
        UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);

        return transform->includeChildBounds;
    }
    void BaseElement::SetExpandBoundsToChildren(bool expand)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);

        transform->includeChildBounds = expand;
        UIUtils::Transform::UpdateBounds(registry, transform);
    }

    const bool BaseElement::IsVisible() const
    {
        UIComponent::Visibility* visibility = &ServiceLocator::GetUIRegistry()->get<UIComponent::Visibility>(_entityId);
        return UIUtils::Visibility::IsVisible(visibility);
    }
    const bool BaseElement::IsLocallyVisible() const
    {
        UIComponent::Visibility* visibility = &ServiceLocator::GetUIRegistry()->get<UIComponent::Visibility>(_entityId);
        return visibility->visible;
    }
    const bool BaseElement::IsParentVisible() const
    {
        UIComponent::Visibility* visibility = &ServiceLocator::GetUIRegistry()->get<UIComponent::Visibility>(_entityId);
        return visibility->parentVisible;
    }
    void BaseElement::SetVisible(bool visible)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Visibility* visibility = &registry->get<UIComponent::Visibility>(_entityId);

        // Don't do anything if visibility doesn't change.
        if (visibility->visible == visible || !UIUtils::Visibility::UpdateVisibility(visibility, visible))
            return;

        UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);

        const bool newVisibility = UIUtils::Visibility::IsVisible(visibility);
        UIUtils::Visibility::UpdateChildVisibility(registry, transform, newVisibility);

        // Update visibility component.
        if (newVisibility)
            registry->emplace<UIComponent::Visible>(_entityId);
        else
            registry->remove<UIComponent::Visible>(_entityId);
    }

    void BaseElement::SetCollisionEnabled(bool enabled)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        // Check if collision enabled state is the same as what we are trying to set. If so doing anything would be redundant.
        if (registry->has<UIComponent::Collidable>(_entityId) == enabled)
            return;

        if (enabled)
            registry->emplace<UIComponent::Collidable>(_entityId);
        else
            registry->remove<UIComponent::Collidable>(_entityId);
    }

    void BaseElement::Destroy()
    {
        UISingleton::UIDataSingleton& dataSingleton = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>();
        dataSingleton.DestroyWidget(_entityId);
    }

    void BaseElement::MarkDirty(entt::registry* registry, entt::entity entId)
    {
        if (!registry->has<UIComponent::Dirty>(entId))
            registry->emplace<UIComponent::Dirty>(entId);
    }
}
