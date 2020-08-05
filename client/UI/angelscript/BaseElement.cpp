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
    // Returns true if visibility changed.
    bool UpdateVisibility(UIComponent::Visibility& visibility, bool visible)
    {
        if (visibility.visible == visible)
            return false;

        const bool oldVisibility = visibility.parentVisible && visibility.visible;
        const bool newVisibility = visibility.parentVisible && visible;
        visibility.visible = visible;

        return oldVisibility != visible;
    }

    // Returns true if visibility changed.
    bool UpdateParentVisibility(UIComponent::Visibility& visibility, bool parentVisible)
    {
        if (visibility.parentVisible == parentVisible)
            return false;

        const bool oldVisibility = visibility.parentVisible && visibility.visible;
        const bool newVisibility = parentVisible && visibility.visible;
        visibility.parentVisible = parentVisible;

        return oldVisibility != newVisibility;
    }

    BaseElement::BaseElement(UI::UIElementType elementType) : _elementType(elementType)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        _entityId = registry->ctx<UISingleton::UIEntityPoolSingleton>().GetId();
        registry->ctx<UISingleton::UIAddElementQueueSingleton>().elementPool.enqueue({ _entityId, elementType, this });
        registry->ctx<UISingleton::UIDataSingleton>().entityToAsObject[_entityId] = this;
    }

    void BaseElement::SetTransform(const vec2& position, const vec2& size)
    {
        const bool hasParent = _transform.parent;
        if (hasParent)
            _transform.localPosition = position;
        else
            _transform.position = position;

        // Don't change size if we are trying to fill parent size since it will just adjust to the parent.
        if (!_transform.fillParentSize)
            _transform.size = size;

        UISingleton::UIDataSingleton& uiDataSingleton = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>();
        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::entity entId = _entityId;
        //ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([position, size, hasParent, entId]()
        //    {
        //        entt::registry* registry = ServiceLocator::GetUIRegistry();
        //        UIComponent::Transform& transform = registry->get<Transform>(entId);

        //        if (hasParent)
        //            transform.localPosition = position;
        //        else
        //            transform.position = position;

        //        // Don't change size if we are trying to fill parent size since it will just adjust to the parent.
        //        if (!transform.fillParentSize)
        //            transform.size = size;

        //        UpdateChildTransforms(registry, transform);

        //        MarkDirty(registry, entId);
        //    });
    }

    void BaseElement::SetPosition(const vec2& position)
    {
        const bool hasParent = _transform.parent;
        if (hasParent)
            _transform.localPosition = position;
        else
            _transform.position = position;

        UISingleton::UIDataSingleton& uiDataSingleton = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>();
        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        /*gameRegistry->ctx<ScriptSingleton>().AddTransaction([position, hasParent, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIComponent::Transform& transform = registry->get<Transform>(entId);

                if (hasParent)
                    transform.localPosition = position;
                else
                    transform.position = position;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);
            });*/
    }

    void BaseElement::SetAnchor(const vec2& anchor)
    {
        _transform.anchor = anchor;

        UISingleton::UIDataSingleton& uiDataSingleton = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>();
        auto& entityToAsObject = uiDataSingleton.entityToAsObject;
        if (auto entityToAsObjectIterator = entityToAsObject.find(entt::entity(_transform.parent)); entityToAsObjectIterator != entityToAsObject.end())
        {
            UIComponent::Transform& parent = entityToAsObjectIterator->getSecond()->_transform;

            _transform.position = UIUtils::Transform::GetAnchorPosition(parent, _transform.anchor);
        }

        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([anchor, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIComponent::Transform& transform = registry->get<Transform>(entId);

                transform.anchor = anchor;

                if (transform.parent)
                {
                    UIComponent::Transform& parent = registry->get<Transform>(entId);

                    transform.position = UIUtils::Transform::GetAnchorPosition(parent, transform.anchor);
                }

                UpdateChildTransforms(registry, transform);
                
                MarkDirty(registry, entId);
            });*/
    }

    void BaseElement::SetLocalAnchor(const vec2& localAnchor)
    {
        _transform.localAnchor = localAnchor;

        UpdateChildTransformsAngelScript(ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>(), _transform);

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([localAnchor, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIComponent::Transform& transform = registry->get<Transform>(entId);

                transform.localAnchor = localAnchor;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);
            });*/
    }

    void BaseElement::SetSize(const vec2& size)
    {
        _transform.size = size;

        UpdateChildTransformsAngelScript(ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>(), _transform);

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([size, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIComponent::Transform& transform = registry->get<Transform>(entId);

                transform.size = size;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);
            });*/
    }

    void BaseElement::SetFillParentSize(bool fillParent)
    {
        //Check so we are actually changing the state.
        if (fillParent == _transform.fillParentSize)
            return;

        _transform.fillParentSize = fillParent;

        if (_transform.parent && fillParent)
        {
            auto& entityToAsObject = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>().entityToAsObject;
            if (auto itr = entityToAsObject.find(entt::entity(_transform.parent)); itr != entityToAsObject.end())
                _transform.size = itr->second->GetSize();
        }

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([fillParent, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIComponent::Transform& transform = registry->get<Transform>(entId);

                transform.fillParentSize = fillParent;

                if (transform.parent && fillParent)
                {
                    UIComponent::Transform& parentTransform = registry->get<Transform>(entt::entity(transform.parent));

                    transform.size = parentTransform.size;

                    UpdateChildTransforms(registry, transform);
                    MarkDirty(registry, entId);
                }
            });*/
    }

    void BaseElement::SetDepth(const u16 depth)
    {
        _transform.sortData.depth = depth;

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([depth, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIComponent::Transform& transform = registry->get<Transform>(entId);

                transform.sortData.depth = depth;

                MarkDirty(registry, entId);
            });*/
    }

    void BaseElement::SetParent(BaseElement* parent)
    {
        UISingleton::UIDataSingleton& uiDataSingleton = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>();

        // Remove old parent.
        if (_transform.parent)
        {
            // Find parent transform as object.
            auto itr = uiDataSingleton.entityToAsObject.find(entt::entity(_transform.parent));
            if (itr != uiDataSingleton.entityToAsObject.end())
            {
                UIComponent::Transform& oldParentTransform = itr->getSecond()->_transform;

                UIUtils::Transform::RemoveChild(oldParentTransform, _entityId);
            }

            // Keep same screen position.
            _transform.position = UIUtils::Transform::GetScreenPosition(_transform);
        }

        // Set new parent.
        _transform.parent = entt::to_integral(parent->_entityId);

        // Calculate origin.
        UIComponent::Transform& parentTransform = parent->_transform;
        vec2 NewOrigin = UIUtils::Transform::GetAnchorPosition(parentTransform, _transform.anchor);

        // Recalculate new local position whilst keeping screen position.
        _transform.localPosition = _transform.position - NewOrigin;
        _transform.position = NewOrigin;

        if (_transform.fillParentSize)
            _transform.size = parentTransform.size;

        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        // Add ourselves to parent's angelscript object children
        UIUtils::Transform::AddChild(parentTransform, _entityId, _elementType);

        // Update visibility
        UIComponent::Visibility& parentVisibility = parent->_visibility;
        if(UpdateParentVisibility(_visibility, parentVisibility.parentVisible && parentVisibility.visible))
            UpdateChildVisibilityAngelScript(uiDataSingleton, _transform, _visibility.parentVisible && _visibility.visible);

        // TRANSACTION
        entt::entity parentEntityId = parent->GetEntityId();
        entt::entity entId = _entityId;
        UI::UIElementType elementType = _elementType;
        vec2 newPosition = _transform.position;
        vec2 newLocalPosition = _transform.localPosition;
        //ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([parentEntityId, entId, elementType, newPosition, newLocalPosition]()
        //    {
        //        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        //        UIComponent::Transform& transform = uiRegistry->get<Transform>(entId);
        //        UIComponent::Transform& parentTransform = uiRegistry->get<Transform>(parentEntityId);

        //        // Remove old parent.
        //        if (transform.parent)
        //        {
        //            UIComponent::Transform& oldParentTransform = uiRegistry->get<Transform>(entt::entity(transform.parent));

        //            //Remove from parents children.
        //            UIUtils::Transform::RemoveChild(oldParentTransform, entId);
        //        }

        //        // Set new parent.
        //        transform.parent = entt::to_integral(parentEntityId);

        //        // Apply calculated new positions
        //        transform.position = newPosition;
        //        transform.localPosition = newLocalPosition;

        //        // Add this to parent's children.
        //        UIUtils::Transform::AddChild(parentTransform, entId, elementType);

        //        if (transform.fillParentSize)
        //            transform.size = parentTransform.size;

        //        UpdateChildTransforms(uiRegistry, transform);

        //        // Update visibility.
        //        UIComponent::Visibility& visibility = uiRegistry->get<Visibility>(entId);
        //        if (UpdateParentVisibility(visibility, uiRegistry->has<Visible>(parentEntityId)))
        //        {
        //            const bool newVisibility = visibility.visible && visibility.parentVisible;
        //            UpdateChildVisibility(uiRegistry, transform, newVisibility);

        //            if (newVisibility)
        //                uiRegistry->emplace<Visible>(entId);
        //            else
        //                uiRegistry->remove<Visible>(entId);
        //        }

        //        MarkDirty(uiRegistry, entId);
        //    });
    }

    void BaseElement::UnsetParent()
    {
        if (!_transform.parent)
            return;
    
        UIUtils::Transform::RemoveParent(_transform);

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIUtils::Transform::RemoveParent(registry->get<Transform>(entId));
            });*/
    }

    void BaseElement::SetExpandBoundsToChildren(bool expand)
    {
        _transform.includeChildBounds = expand;

        // TRANSACTION
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([expand, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIComponent::Transform& transform = registry->get<Transform>(entId);
                transform.includeChildBounds = expand;

                UpdateChildBounds(registry, transform);
            });*/
    }

    void BaseElement::SetVisible(bool visible)
    {
        //Don't do anything if new visibility isn't different from the old one.
        if (_visibility.visible == visible)
            return;

        if(UpdateVisibility(_visibility, visible))
            UpdateChildVisibilityAngelScript(ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>(), _transform, _visibility.parentVisible && _visibility.visible);

        // TRANSACTION
        entt::entity entId = _entityId;
        //ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([visible, entId]()
        //    {
        //        entt::registry* registry = ServiceLocator::GetUIRegistry();
        //        UIComponent::Visibility& visibility = registry->get<Visibility>(entId);

        //        if (!UpdateVisibility(visibility, visible))
        //            return;

        //        const bool newVisibility = visibility.parentVisible && visibility.visible;
        //        UpdateChildVisibility(registry, registry->get<Transform>(entId), newVisibility);

        //        // Update visibility component.
        //        if (newVisibility)
        //            registry->emplace<Visible>(entId);
        //        else
        //            registry->remove<Visible>(entId);
        //    });
    }

    void BaseElement::SetCollisionEnabled(bool enabled)
    {
        entt::entity entId = _entityId;
        //ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([enabled, entId]()
        //    {
        //        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        //        
        //        // Check if collision enabled state is the same as what we are trying to set. If so doing anything would be redundant.
        //        if (uiRegistry->has<Collidable>(entId) == enabled)
        //            return;

        //        if (enabled)
        //            uiRegistry->emplace<Collidable>(entId);
        //        else
        //            uiRegistry->remove<Collidable>(entId);
        //    });
    }

    void BaseElement::Destroy()
    {
        entt::entity entId = _entityId;
        /*ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

                dataSingleton.DestroyWidget(entId);
            });*/
    }

    void BaseElement::MarkDirty(entt::registry* registry, entt::entity entId)
    {
        if (!registry->has<UIComponent::Dirty>(entId))
            registry->emplace<UIComponent::Dirty>(entId);
    }

    void BaseElement::UpdateChildTransforms(entt::registry* uiRegistry, UIComponent::Transform& parent)
    {
        ZoneScoped;
        for (UI::UIChild& child : parent.children)
        {
            entt::entity entId = entt::entity(child.entity);
            UIComponent::Transform& childTransform = uiRegistry->get<UIComponent::Transform>(entId);

            childTransform.position = UIUtils::Transform::GetAnchorPosition(parent, childTransform.anchor);
            if (childTransform.fillParentSize) childTransform.size = parent.size;

            UpdateChildTransforms(uiRegistry, childTransform);
            MarkDirty(uiRegistry, entId);
        }

        UpdateChildBounds(uiRegistry, parent);
    }
    void BaseElement::UpdateChildTransformsAngelScript(UISingleton::UIDataSingleton& uiDataSingleton, UIComponent::Transform& parent)
    {
        ZoneScoped;
        for (UI::UIChild& child : parent.children)
        {
            auto itr = uiDataSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (itr == uiDataSingleton.entityToAsObject.end())
                continue;

            UIComponent::Transform& childTransform = itr->getSecond()->_transform;

            childTransform.position = UIUtils::Transform::GetAnchorPosition(parent, childTransform.anchor);
            if (childTransform.fillParentSize) childTransform.size = parent.size;

            UpdateChildTransformsAngelScript(uiDataSingleton, childTransform);
        }
    }

    void BaseElement::UpdateChildVisibility(entt::registry* uiRegistry, const UIComponent::Transform& parent, bool parentVisibility)
    {
        ZoneScoped;
        for (const UI::UIChild& child : parent.children)
        {
            const entt::entity childEntity = entt::entity(child.entity);
            UIComponent::Visibility& childVisibility = uiRegistry->get<UIComponent::Visibility>(childEntity);

            if (!UpdateParentVisibility(childVisibility, parentVisibility))
                continue;

            const bool newVisibility = childVisibility.parentVisible && childVisibility.visible;
            UpdateChildVisibility(uiRegistry, uiRegistry->get<UIComponent::Transform>(childEntity), newVisibility);

            if (newVisibility)
                uiRegistry->emplace<UIComponent::Visible>(entt::entity(child.entity));
            else
                uiRegistry->remove<UIComponent::Visible>(entt::entity(child.entity));
        }
    }
    void BaseElement::UpdateChildVisibilityAngelScript(UISingleton::UIDataSingleton& uiDataSingleton, const UIComponent::Transform& parent, bool parentVisibility)
    {
        ZoneScoped;
        for (const UI::UIChild& child : parent.children)
        {
            auto iterator = uiDataSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (iterator == uiDataSingleton.entityToAsObject.end())
                continue;

            BaseElement* asChild = iterator->getSecond();
            UIComponent::Visibility& childVisibility = asChild->_visibility;

            if (!UpdateParentVisibility(childVisibility, parentVisibility))
                continue;

            const bool newVisibility = childVisibility.parentVisible && childVisibility.visible;
            UpdateChildVisibilityAngelScript(uiDataSingleton, asChild->_transform, newVisibility);
        }
    }

    void BaseElement::UpdateChildBounds(entt::registry* uiRegistry, UIComponent::Transform& transform)
    {
        ZoneScoped;
        transform.minBound = UIUtils::Transform::GetMinBounds(transform);
        transform.maxBound = UIUtils::Transform::GetMaxBounds(transform);

        if (transform.includeChildBounds)
        {
            for (const UI::UIChild& child : transform.children)
            {
                UpdateChildBounds(uiRegistry, uiRegistry->get<UIComponent::Transform>(entt::entity(child.entity)));
            }
        }

        if (!transform.parent)
            return;

        UpdateBounds(uiRegistry, uiRegistry->get<UIComponent::Transform>(entt::entity(transform.parent)));
    }
    void BaseElement::UpdateBounds(entt::registry* uiRegistry, UIComponent::Transform& transform)
    {
        ZoneScoped;
        transform.minBound = UIUtils::Transform::GetMinBounds(transform);
        transform.maxBound = UIUtils::Transform::GetMaxBounds(transform);

        if (transform.includeChildBounds)
        {
            for (const UI::UIChild& child : transform.children)
            {
                UIComponent::Transform& childTransform = uiRegistry->get<UIComponent::Transform>(entt::entity(child.entity));

                if (childTransform.minBound.x < transform.minBound.x) { transform.minBound.x = childTransform.minBound.x; }
                if (childTransform.minBound.y < transform.minBound.y) { transform.minBound.y = childTransform.minBound.y; }

                if (childTransform.maxBound.x > transform.maxBound.x) { transform.maxBound.x = childTransform.maxBound.x; }
                if (childTransform.maxBound.y > transform.maxBound.y) { transform.maxBound.y = childTransform.maxBound.y; }
            }
        }

        if (!transform.parent)
            return;

        UIComponent::Transform& parentTransform = uiRegistry->get<UIComponent::Transform>(entt::entity(transform.parent));
        if(parentTransform.includeChildBounds)
            UpdateBounds(uiRegistry, parentTransform);
    }
}
