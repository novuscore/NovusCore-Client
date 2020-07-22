#include "asUITransform.h"
#include "../../../Utils/ServiceLocator.h"

#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/Singletons/UIDataSingleton.h"
#include "../../../ECS/Components/UI/Singletons/UIAddElementQueueSingleton.h"

#include "../../../ECS/Components/UI/UIVisible.h"
#include "../../../ECS/Components/UI/UICollidable.h"
#include "../../../ECS/Components/UI/UIDirty.h"
#include "../../../UI/UITransformUtils.h"

namespace UI
{
    // Returns true if visibility changed.
    bool UpdateVisibility(UIVisibility& visibility, bool visible)
    {
        if (visibility.visible == visible)
            return false;

        const bool oldVisibility = visibility.parentVisible && visibility.visible;
        const bool newVisibility = visibility.parentVisible && visible;
        visibility.visible = visible;

        return oldVisibility != visible;
    }

    // Returns true if visibility changed.
    bool UpdateParentVisibility(UIVisibility& visibility, bool parentVisible)
    {
        if (visibility.parentVisible == parentVisible)
            return false;

        const bool oldVisibility = visibility.parentVisible && visibility.visible;
        const bool newVisibility = parentVisible && visibility.visible;
        visibility.parentVisible = parentVisible;

        return oldVisibility != newVisibility;
    }

    asUITransform::asUITransform(entt::entity entityId, UIElementType elementType) : _entityId(entityId), _elementType(elementType)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UIElementCreationData elementData{ entityId, elementType, this };

        UIAddElementQueueSingleton& addElementQueue = registry->ctx<UIAddElementQueueSingleton>();
        addElementQueue.elementPool.enqueue(elementData);

        UIDataSingleton& uiDataSingleton = registry->ctx<UIDataSingleton>();
        uiDataSingleton.entityToAsObject[entityId] = this;
    }

    void asUITransform::SetTransform(const vec2& position, const vec2& size)
    {
        const bool hasParent = _transform.parent;
        if (hasParent)
            _transform.localPosition = position;
        else
            _transform.position = position;

        // Don't change size if we are trying to fill parent size since it will just adjust to the parent.
        if (!_transform.fillParentSize)
            _transform.size = size;

        UIDataSingleton& uiDataSingleton = ServiceLocator::GetUIRegistry()->ctx<UIDataSingleton>();
        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([position, size, hasParent, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                if (hasParent)
                    transform.localPosition = position;
                else
                    transform.position = position;

                // Don't change size if we are trying to fill parent size since it will just adjust to the parent.
                if (!transform.fillParentSize)
                    transform.size = size;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);
            });
    }

    void asUITransform::SetPosition(const vec2& position)
    {
        const bool hasParent = _transform.parent;
        if (hasParent)
            _transform.localPosition = position;
        else
            _transform.position = position;

        UIDataSingleton& uiDataSingleton = ServiceLocator::GetUIRegistry()->ctx<UIDataSingleton>();
        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([position, hasParent, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                if (hasParent)
                    transform.localPosition = position;
                else
                    transform.position = position;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);
            });
    }

    void asUITransform::SetAnchor(const vec2& anchor)
    {
        _transform.anchor = anchor;

        UIDataSingleton& uiDataSingleton = ServiceLocator::GetUIRegistry()->ctx<UIDataSingleton>();
        auto& entityToAsObject = uiDataSingleton.entityToAsObject;
        if (auto entityToAsObjectIterator = entityToAsObject.find(entt::entity(_transform.parent)); entityToAsObjectIterator != entityToAsObject.end())
        {
            UITransform& parent = entityToAsObjectIterator->getSecond()->_transform;

            _transform.position = UITransformUtils::GetAnchorPosition(parent, _transform.anchor);
        }

        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([anchor, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                transform.anchor = anchor;

                if (transform.parent)
                {
                    UITransform& parent = registry->get<UITransform>(entId);

                    transform.position = UITransformUtils::GetAnchorPosition(parent, transform.anchor);
                }

                UpdateChildTransforms(registry, transform);
                
                MarkDirty(registry, entId);
            });
    }

    void asUITransform::SetLocalAnchor(const vec2& localAnchor)
    {
        _transform.localAnchor = localAnchor;

        UpdateChildTransformsAngelScript(ServiceLocator::GetUIRegistry()->ctx<UIDataSingleton>(), _transform);

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([localAnchor, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                transform.localAnchor = localAnchor;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);
            });
    }

    void asUITransform::SetSize(const vec2& size)
    {
        _transform.size = size;

        UpdateChildTransformsAngelScript(ServiceLocator::GetUIRegistry()->ctx<UIDataSingleton>(), _transform);

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([size, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                transform.size = size;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);
            });
    }

    void asUITransform::SetFillParentSize(bool fillParent)
    {
        //Check so we are actually changing the state.
        if (fillParent == _transform.fillParentSize)
            return;

        _transform.fillParentSize = fillParent;

        if (_transform.parent && fillParent)
        {
            auto& entityToAsObject = ServiceLocator::GetUIRegistry()->ctx<UIDataSingleton>().entityToAsObject;
            if (auto itr = entityToAsObject.find(entt::entity(_transform.parent)); itr != entityToAsObject.end())
                _transform.size = itr->second->GetSize();
        }

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([fillParent, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                transform.fillParentSize = fillParent;

                if (transform.parent && fillParent)
                {
                    UITransform& parentTransform = registry->get<UITransform>(entt::entity(transform.parent));

                    transform.size = parentTransform.size;

                    UpdateBounds(registry, transform);
                    MarkDirty(registry, entId);
                }
            });
    }

    void asUITransform::SetDepth(const u16 depth)
    {
        _transform.sortData.depth = depth;

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([depth, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                transform.sortData.depth = depth;

                MarkDirty(registry, entId);
            });
    }

    void asUITransform::SetParent(asUITransform* parent)
    {
        UIDataSingleton& uiDataSingleton = ServiceLocator::GetUIRegistry()->ctx<UIDataSingleton>();

        // Remove old parent.
        if (_transform.parent)
        {
            // Find parent transform as object.
            auto entityToAsObjectIterator = uiDataSingleton.entityToAsObject.find(entt::entity(_transform.parent));
            if (entityToAsObjectIterator != uiDataSingleton.entityToAsObject.end())
            {
                UITransform& oldParentTransform = entityToAsObjectIterator->getSecond()->_transform;

                UITransformUtils::RemoveChild(oldParentTransform, _entityId);
            }

            // Keep same screen position.
            _transform.position = UITransformUtils::GetScreenPosition(_transform);
        }

        // Set new parent.
        _transform.parent = entt::to_integral(parent->_entityId);

        // Calculate origin.
        UITransform& parentTransform = parent->_transform;
        vec2 NewOrigin = UITransformUtils::GetAnchorPosition(parentTransform, _transform.anchor);

        // Recalculate new local position whilst keeping screen position.
        _transform.localPosition = (NewOrigin + parentTransform.localPosition) - _transform.position;
        _transform.position = NewOrigin;

        if (_transform.fillParentSize)
            _transform.size = parentTransform.size;

        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        // Add ourselves to parent's angelscript object children
        UITransformUtils::AddChild(parentTransform, _entityId, _elementType);

        // Update visibility
        UIVisibility& parentVisibility = parent->_visibility;
        if(UpdateParentVisibility(_visibility, parentVisibility.parentVisible && parentVisibility.visible))
            UpdateChildVisibilityAngelScript(uiDataSingleton, _transform, _visibility.parentVisible && _visibility.visible);

        // TRANSACTION
        entt::entity parentEntityId = parent->GetEntityId();
        entt::entity entId = _entityId;
        UIElementType elementType = _elementType;
        vec2 newPosition = _transform.position;
        vec2 newLocalPosition = _transform.localPosition;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([parentEntityId, entId, elementType, newPosition, newLocalPosition]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);
                UITransform& parentTransform = uiRegistry->get<UITransform>(parentEntityId);
                

                // Remove old parent.
                if (transform.parent)
                {
                    UITransform& oldParentTransform = uiRegistry->get<UITransform>(entt::entity(transform.parent));

                    //Remove from parents children.
                    UITransformUtils::RemoveChild(oldParentTransform, entId);
                }

                // Set new parent.
                transform.parent = entt::to_integral(parentEntityId);

                // Apply calculated new positions
                transform.position = newPosition;
                transform.localPosition = newLocalPosition;

                // Add this to parent's children.
                UITransformUtils::AddChild(parentTransform, entId, elementType);

                if (transform.fillParentSize)
                    transform.size = parentTransform.size;

                UpdateChildTransforms(uiRegistry, transform);

                // Update visibility.
                UIVisibility& visibility = uiRegistry->get<UIVisibility>(entId);
                if (UpdateParentVisibility(visibility, uiRegistry->has<UIVisible>(parentEntityId)))
                {
                    const bool newVisibility = visibility.visible && visibility.parentVisible;
                    UpdateChildVisibility(uiRegistry, transform, newVisibility);

                    if (newVisibility)
                        uiRegistry->emplace<UIVisible>(entId);
                    else
                        uiRegistry->remove<UIVisible>(entId);
                }

                MarkDirty(uiRegistry, entId);
            });
    }

    void asUITransform::SetExpandBoundsToChildren(bool expand)
    {
        _transform.includeChildBounds = expand;

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([expand, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);
                transform.includeChildBounds = expand;

                UpdateBounds(registry, transform);
            });
    }

    void asUITransform::SetVisible(bool visible)
    {
        //Don't do anything if new visibility isn't different from the old one.
        if (_visibility.visible == visible)
            return;

        if(UpdateVisibility(_visibility, visible))
            UpdateChildVisibilityAngelScript(ServiceLocator::GetUIRegistry()->ctx<UIDataSingleton>(), _transform, _visibility.parentVisible && _visibility.visible);

        // TRANSACTION
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([visible, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIVisibility& visibility = registry->get<UIVisibility>(entId);

                if (!UpdateVisibility(visibility, visible))
                    return;

                const bool newVisibility = visibility.parentVisible && visibility.visible;
                UpdateChildVisibility(registry, registry->get<UITransform>(entId), newVisibility);

                // Update visibility component.
                if (newVisibility)
                    registry->emplace<UIVisible>(entId);
                else
                    registry->remove<UIVisible>(entId);
            });
    }

    void asUITransform::SetCollisionEnabled(bool enabled)
    {
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([enabled, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                
                // Check if collision enabled state is the same as what we are trying to set. If so doing anything would be redundant.
                if (uiRegistry->has<UICollidable>(entId) == enabled)
                    return;

                if (enabled)
                    uiRegistry->emplace<UICollidable>(entId);
                else
                    uiRegistry->remove<UICollidable>(entId);
            });
    }

    void asUITransform::Destroy()
    {
        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UIDataSingleton& dataSingleton = registry->ctx<UIDataSingleton>();

                dataSingleton.DestroyWidget(entId);
            });
    }

    void asUITransform::MarkDirty(entt::registry* registry, entt::entity entId)
    {
        if (!registry->has<UIDirty>(entId))
            registry->emplace<UIDirty>(entId);
    }

    void asUITransform::UpdateChildTransforms(entt::registry* uiRegistry, UITransform& parent)
    {
        for (UIChild& child : parent.children)
        {
            entt::entity entId = entt::entity(child.entity);
            UITransform& childTransform = uiRegistry->get<UITransform>(entId);

            childTransform.position = UITransformUtils::GetAnchorPosition(parent, childTransform.anchor);
            if (childTransform.fillParentSize) childTransform.size = parent.size;

            UpdateChildTransforms(uiRegistry, childTransform);

            MarkDirty(uiRegistry, entId);
        }

        UpdateBounds(uiRegistry, parent);
    }
    void asUITransform::UpdateChildTransformsAngelScript(UI::UIDataSingleton& uiDataSingleton, UITransform& parent)
    {
        for (UIChild& child : parent.children)
        {
            auto itr = uiDataSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (itr == uiDataSingleton.entityToAsObject.end())
                continue;

            UITransform& childTransform = itr->getSecond()->_transform;

            childTransform.position = UITransformUtils::GetAnchorPosition(parent, childTransform.anchor);
            if (childTransform.fillParentSize) childTransform.size = parent.size;

            UpdateChildTransformsAngelScript(uiDataSingleton, childTransform);
        }
    }

    void asUITransform::UpdateChildVisibility(entt::registry* uiRegistry, const UITransform& parent, bool parentVisibility)
    {
        for (const UIChild& child : parent.children)
        {
            const entt::entity childEntity = entt::entity(child.entity);
            UIVisibility& childVisibility = uiRegistry->get<UIVisibility>(childEntity);

            if (!UpdateParentVisibility(childVisibility, parentVisibility))
                continue;

            const bool newVisibility = childVisibility.parentVisible && childVisibility.visible;
            UpdateChildVisibility(uiRegistry, uiRegistry->get<UITransform>(childEntity), newVisibility);

            if (newVisibility)
                uiRegistry->emplace<UIVisible>(entt::entity(child.entity));
            else
                uiRegistry->remove<UIVisible>(entt::entity(child.entity));
        }
    }
    void asUITransform::UpdateChildVisibilityAngelScript(UI::UIDataSingleton& uiDataSingleton, const UITransform& parent, bool parentVisibility)
    {
        for (const UIChild& child : parent.children)
        {
            auto iterator = uiDataSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (iterator == uiDataSingleton.entityToAsObject.end())
                continue;

            asUITransform* asChild = iterator->getSecond();
            UIVisibility& childVisibility = asChild->_visibility;

            if (!UpdateParentVisibility(childVisibility, parentVisibility))
                continue;

            const bool newVisibility = childVisibility.parentVisible && childVisibility.visible;
            UpdateChildVisibilityAngelScript(uiDataSingleton, asChild->_transform, newVisibility);
        }
    }

    void asUITransform::UpdateBounds(entt::registry* uiRegistry, UITransform& transform)
    {
        transform.minBound = UITransformUtils::GetMinBounds(transform);
        transform.maxBound = UITransformUtils::GetMaxBounds(transform);

        if (transform.includeChildBounds)
        {
            for (const UIChild& child : transform.children)
            {
                UpdateBounds(uiRegistry, uiRegistry->get<UITransform>(entt::entity(child.entity)));
            }
        }

        if (!transform.parent)
            return;

        ShallowUpdateBounds(uiRegistry, uiRegistry->get<UITransform>(entt::entity(transform.parent)));
    }
    void asUITransform::ShallowUpdateBounds(entt::registry* uiRegistry, UITransform& parent)
    {
        parent.minBound = UITransformUtils::GetMinBounds(parent);
        parent.maxBound = UITransformUtils::GetMaxBounds(parent);

        if (!parent.includeChildBounds)
            return;

        for (const UIChild& child : parent.children)
        {
            UITransform& childTransform = uiRegistry->get<UITransform>(entt::entity(child.entity));

            if (childTransform.minBound.x < parent.minBound.x) { parent.minBound.x = childTransform.minBound.x; }
            if (childTransform.minBound.y < parent.minBound.y) { parent.minBound.y = childTransform.minBound.y; }

            if (childTransform.maxBound.x > parent.maxBound.x) { parent.maxBound.x = childTransform.maxBound.x; }
            if (childTransform.maxBound.y > parent.maxBound.y) { parent.maxBound.y = childTransform.maxBound.y; }
        }

        if (parent.parent)
            ShallowUpdateBounds(uiRegistry, uiRegistry->get<UITransform>(entt::entity(parent.parent)));
    }
}
