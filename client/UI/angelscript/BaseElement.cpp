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

namespace UIScripting
{
    BaseElement::BaseElement(UI::UIElementType elementType) : _elementType(elementType)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        _entityId = registry->ctx<UISingleton::UIEntityPoolSingleton>().GetId();
        registry->ctx<UISingleton::UIDataSingleton>().entityToAsObject[_entityId] = this;
    }

    // TODO: Move to UIUtils::Visibility
    // Returns true if visibility changed.
    bool UpdateVisibility(UIComponent::Visibility* visibility, bool visible)
    {
        if (visibility->visible == visible)
            return false;

        const bool oldVisibility = visibility->parentVisible && visibility->visible;
        const bool newVisibility = visibility->parentVisible && visible;
        visibility->visible = visible;

        return oldVisibility != visible;
    }

    // TODO: Move to UIUtils::Visibility
    // Returns true if visibility changed.
    bool UpdateParentVisibility(UIComponent::Visibility* visibility, bool parentVisible)
    {
        if (visibility->parentVisible == parentVisible)
            return false;

        const bool oldVisibility = visibility->parentVisible && visibility->visible;
        const bool newVisibility = parentVisible && visibility->visible;
        visibility->parentVisible = parentVisible;

        return oldVisibility != newVisibility;
    }

    void BaseElement::SetTransform(const vec2& position, const vec2& size)
    {
        const bool hasParent = _transform->parent;
        if (hasParent)
            _transform->localPosition = position;
        else
            _transform->position = position;

        // Don't change size if we are trying to fill parent size since it will just adjust to the parent.
        if (!_transform->fillParentSize)
            _transform->size = size;

        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UpdateChildTransforms(registry, _transform);
        MarkDirty(registry, _entityId);
    }

    void BaseElement::SetPosition(const vec2& position)
    {
        const bool hasParent = _transform->parent;
        if (hasParent)
            _transform->localPosition = position;
        else
            _transform->position = position;

        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UpdateChildTransforms(registry, _transform);
        MarkDirty(registry, _entityId);
    }

    void BaseElement::SetAnchor(const vec2& anchor)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        _transform->anchor = anchor;

        if (_transform->parent)
        {
            UIComponent::Transform* parent = &registry->get<UIComponent::Transform>(entt::entity(_transform->parent));
            _transform->position = UIUtils::Transform::GetAnchorPosition(parent, _transform->anchor);
        }

        UpdateChildTransforms(registry, _transform);
        MarkDirty(registry, _entityId);
    }

    void BaseElement::SetLocalAnchor(const vec2& localAnchor)
    {
        _transform->localAnchor = localAnchor;

        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UpdateChildTransforms(registry, _transform);
        MarkDirty(registry, _entityId);
    }

    void BaseElement::SetSize(const vec2& size)
    {
        _transform->size = size;

        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UpdateChildTransforms(registry, _transform);
        MarkDirty(registry, _entityId);
    }

    void BaseElement::SetFillParentSize(bool fillParent)
    {
        //Check so we are actually changing the state.
        if (fillParent == _transform->fillParentSize)
            return;

        entt::registry* registry = ServiceLocator::GetUIRegistry();

        _transform->fillParentSize = fillParent;

        if (_transform->parent && fillParent)
        {
            UIComponent::Transform* parentTransform = &registry->get<UIComponent::Transform>(entt::entity(_transform->parent));

            _transform->size = parentTransform->size;

            UpdateChildTransforms(registry, _transform);
            MarkDirty(registry, _entityId);
        }
    }

    void BaseElement::SetDepth(const u16 depth)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        _transform->sortData.depth = depth;
        MarkDirty(registry, _entityId);
    }

    void BaseElement::SetParent(BaseElement* parent)
    {
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();

        // Remove old parent.
        if (_transform->parent)
        {
            UIComponent::Transform* oldParentTransform = &uiRegistry->get<UIComponent::Transform>(entt::entity(_transform->parent));

            //Remove from parents children.
            UIUtils::Transform::RemoveChild(oldParentTransform, _entityId);

            // Keep same screen position.
            _transform->position = UIUtils::Transform::GetScreenPosition(_transform);
        }

        // Set new parent.
        _transform->parent = entt::to_integral(parent->_entityId);

        // Calculate origin.
        UIComponent::Transform* parentTransform = parent->_transform;
        vec2 NewOrigin = UIUtils::Transform::GetAnchorPosition(parentTransform, _transform->anchor);

        // Recalculate new local position whilst keeping screen position.
        _transform->localPosition = _transform->position - NewOrigin;
        _transform->position = NewOrigin;

        if (_transform->fillParentSize)
            _transform->size = parentTransform->size;

        UpdateChildTransforms(uiRegistry, _transform);

        // Add ourselves to parent's angelscript object children
        UIUtils::Transform::AddChild(parentTransform, _entityId, _elementType);

        // Update _visibility->
        UIComponent::Visibility* visibility = &uiRegistry->get<UIComponent::Visibility>(_entityId);
        if (UpdateParentVisibility(visibility, uiRegistry->has<UIComponent::Visible>(parent->_entityId)))
        {
            const bool newVisibility = _visibility->visible && _visibility->parentVisible;
            UpdateChildVisibility(uiRegistry, _transform, newVisibility);

            if (newVisibility)
                uiRegistry->emplace<UIComponent::Visible>(_entityId);
            else
                uiRegistry->remove<UIComponent::Visible>(_entityId);
        }

        MarkDirty(uiRegistry, _entityId);
    }

    void BaseElement::UnsetParent()
    {
        if (!_transform->parent)
            return;
    
        UIUtils::Transform::RemoveParent(_transform);
    }

    void BaseElement::SetExpandBoundsToChildren(bool expand)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        _transform->includeChildBounds = expand;
        UpdateChildBounds(registry, _transform);
    }

    void BaseElement::SetVisible(bool visible)
    {
        // Don't do anything if new visibility isn't different from the old one.
        if (_visibility->visible == visible)
            return;

        if (!UpdateVisibility(_visibility, visible))
            return;

        entt::registry* registry = ServiceLocator::GetUIRegistry();
        const bool newVisibility = _visibility->parentVisible && _visibility->visible;
        UpdateChildVisibility(registry, &registry->get<UIComponent::Transform>(_entityId), newVisibility);

        // Update visibility component.
        if (newVisibility)
            registry->emplace<UIComponent::Visible>(_entityId);
        else
            registry->remove<UIComponent::Visible>(_entityId);
    }

    void BaseElement::SetCollisionEnabled(bool enabled)
    {
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        
        // Check if collision enabled state is the same as what we are trying to set. If so doing anything would be redundant.
        if (uiRegistry->has<UIComponent::Collidable>(_entityId) == enabled)
            return;

        if (enabled)
            uiRegistry->emplace<UIComponent::Collidable>(_entityId);
        else
            uiRegistry->remove<UIComponent::Collidable>(_entityId);
    }

    void BaseElement::Destroy()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        dataSingleton.DestroyWidget(_entityId);
    }

    void BaseElement::MarkDirty(entt::registry* registry, entt::entity entId)
    {
        if (!registry->has<UIComponent::Dirty>(entId))
            registry->emplace<UIComponent::Dirty>(entId);
    }

    void BaseElement::UpdateChildTransforms(entt::registry* uiRegistry, UIComponent::Transform* parent)
    {
        ZoneScoped;
        for (UI::UIChild& child : parent->children)
        {
            entt::entity entId = entt::entity(child.entity);
            UIComponent::Transform* childTransform = &uiRegistry->get<UIComponent::Transform>(entId);

            childTransform->position = UIUtils::Transform::GetAnchorPosition(parent, childTransform->anchor);
            if (childTransform->fillParentSize) 
                childTransform->size = parent->size;

            UpdateChildTransforms(uiRegistry, childTransform);
            MarkDirty(uiRegistry, entId);
        }

        UpdateChildBounds(uiRegistry, parent);
    }

    void BaseElement::UpdateChildVisibility(entt::registry* uiRegistry, const UIComponent::Transform* parent, bool parentVisibility)
    {
        ZoneScoped;
        for (const UI::UIChild& child : parent->children)
        {
            const entt::entity childEntity = entt::entity(child.entity);
            UIComponent::Visibility* childVisibility = &uiRegistry->get<UIComponent::Visibility>(childEntity);

            if (!UpdateParentVisibility(childVisibility, parentVisibility))
                continue;

            const bool newVisibility = childVisibility->parentVisible && childVisibility->visible;
            UpdateChildVisibility(uiRegistry, &uiRegistry->get<UIComponent::Transform>(childEntity), newVisibility);

            if (newVisibility)
                uiRegistry->emplace<UIComponent::Visible>(entt::entity(child.entity));
            else
                uiRegistry->remove<UIComponent::Visible>(entt::entity(child.entity));
        }
    }

    void BaseElement::UpdateChildBounds(entt::registry* uiRegistry, UIComponent::Transform* transform)
    {
        ZoneScoped;
        transform->minBound = UIUtils::Transform::GetMinBounds(transform);
        transform->maxBound = UIUtils::Transform::GetMaxBounds(transform);

        if (transform->includeChildBounds)
        {
            for (const UI::UIChild& child : transform->children)
            {
                UpdateChildBounds(uiRegistry, &uiRegistry->get<UIComponent::Transform>(entt::entity(child.entity)));
            }
        }

        if (!transform->parent)
            return;

        UpdateBounds(uiRegistry, &uiRegistry->get<UIComponent::Transform>(entt::entity(transform->parent)));
    }
    void BaseElement::UpdateBounds(entt::registry* uiRegistry, UIComponent::Transform* transform)
    {
        ZoneScoped;
        transform->minBound = UIUtils::Transform::GetMinBounds(transform);
        transform->maxBound = UIUtils::Transform::GetMaxBounds(transform);

        if (transform->includeChildBounds)
        {
            for (const UI::UIChild& child : transform->children)
            {
                UIComponent::Transform* childTransform = &uiRegistry->get<UIComponent::Transform>(entt::entity(child.entity));

                if (childTransform->minBound.x < transform->minBound.x) { transform->minBound.x = childTransform->minBound.x; }
                if (childTransform->minBound.y < transform->minBound.y) { transform->minBound.y = childTransform->minBound.y; }

                if (childTransform->maxBound.x > transform->maxBound.x) { transform->maxBound.x = childTransform->maxBound.x; }
                if (childTransform->maxBound.y > transform->maxBound.y) { transform->maxBound.y = childTransform->maxBound.y; }
            }
        }

        if (!transform->parent)
            return;

        UIComponent::Transform* parentTransform = &uiRegistry->get<UIComponent::Transform>(entt::entity(transform->parent));
        if(parentTransform->includeChildBounds)
            UpdateBounds(uiRegistry, parentTransform);
    }
}
