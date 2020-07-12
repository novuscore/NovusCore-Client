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
    asUITransform::asUITransform(entt::entity entityId, UIElementType elementType) : _entityId(entityId), _elementType(elementType)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UIElementCreationData elementData{ entityId, elementType, this };

        UIAddElementQueueSingleton& addElementQueue = registry->ctx<UIAddElementQueueSingleton>();
        addElementQueue.elementPool.enqueue(elementData);

        UIDataSingleton& uiDataSingleton = registry->ctx<UIDataSingleton>();
        uiDataSingleton.entityToAsObject[entityId] = this;
    }

    void asUITransform::SetPosition(const vec2& position)
    {
        bool hasParent = _transform.parent;
        if (hasParent)
            _transform.localPosition = position;
        else
            _transform.position = position;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();
        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        _transform.minBound = UITransformUtils::GetMinBounds(_transform);
        _transform.maxBound = UITransformUtils::GetMaxBounds(_transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([position, hasParent, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                if (!registry->has<UIDirty>(entId))
                    registry->emplace<UIDirty>(entId);
                if (hasParent)
                    transform.localPosition = position;
                else
                    transform.position = position;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);

                transform.minBound = UITransformUtils::GetMinBounds(transform);
                transform.maxBound = UITransformUtils::GetMaxBounds(transform);
            });
    }

    void asUITransform::SetAnchor(const vec2& anchor)
    {
        _transform.anchor = anchor;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();
        auto entityToAsObjectIterator = uiDataSingleton.entityToAsObject.find(entt::entity(_transform.parent));
        if (entityToAsObjectIterator != uiDataSingleton.entityToAsObject.end())
        {
            UITransform& parent = entityToAsObjectIterator->getSecond()->_transform;

            _transform.position = UITransformUtils::GetAnchorPosition(parent, _transform.anchor);
        }

        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        _transform.minBound = UITransformUtils::GetMinBounds(_transform);
        _transform.maxBound = UITransformUtils::GetMaxBounds(_transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([anchor, entId]()
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

                transform.minBound = UITransformUtils::GetMinBounds(transform);
                transform.maxBound = UITransformUtils::GetMaxBounds(transform);
            });
    }

    void asUITransform::SetLocalAnchor(const vec2& localAnchor)
    {
        _transform.localAnchor = localAnchor;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();
        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        _transform.minBound = UITransformUtils::GetMinBounds(_transform);
        _transform.maxBound = UITransformUtils::GetMaxBounds(_transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([localAnchor, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                transform.localAnchor = localAnchor;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);

                transform.minBound = UITransformUtils::GetMinBounds(transform);
                transform.maxBound = UITransformUtils::GetMaxBounds(transform);
            });
    }

    void asUITransform::SetSize(const vec2& size)
    {
        _transform.size = size;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();
        UpdateChildTransformsAngelScript(uiDataSingleton, _transform);

        _transform.minBound = UITransformUtils::GetMinBounds(_transform);
        _transform.maxBound = UITransformUtils::GetMaxBounds(_transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([size, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                transform.size = size;

                UpdateChildTransforms(registry, transform);

                MarkDirty(registry, entId);

                transform.minBound = UITransformUtils::GetMinBounds(transform);
                transform.maxBound = UITransformUtils::GetMaxBounds(transform);
            });
    }

    void asUITransform::SetDepth(const u16 depth)
    {
        _transform.sortData.depth = depth;

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([depth, entId]()
            {
                entt::registry* registry = ServiceLocator::GetUIRegistry();
                UITransform& transform = registry->get<UITransform>(entId);

                transform.sortData.depth = depth;

                MarkDirty(registry, entId);
            });
    }

    void asUITransform::SetParent(asUITransform* parent)
    {
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();

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

            // Keep same absolute position.
            _transform.position = _transform.position + _transform.localPosition;
            _transform.localPosition = vec2(0, 0);

            _visibility.parentVisible = true;
        }

        // Set new parent.
        _transform.parent = entt::to_integral(parent->_entityId);

        // Calculate origin.
        UITransform& parentTransform = parent->_transform;
        vec2 NewOrigin = UITransformUtils::GetAnchorPosition(parentTransform, _transform.anchor);

        // Recalculate new local position whilst keeping absolute position.
        _transform.localPosition = (NewOrigin + parentTransform.localPosition) - _transform.position;
        _transform.position = NewOrigin + parentTransform.localPosition;

        // Add ourselves to parent's angelscript object children
        UITransformUtils::AddChild(parentTransform, _entityId, _elementType);

        // Update visibility
        UIVisibility& parentVisibility = parent->_visibility;
        _visibility.parentVisible = parentVisibility.parentVisible && parentVisibility.visible;

        UpdateChildVisibilityAngelScript(uiDataSingleton, _transform, _visibility.parentVisible && _visibility.visible);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity parentEntityId = parent->GetEntityId();
        entt::entity entityId = _entityId;
        UIElementType elementType = _elementType;
        vec2 newPosition = _transform.position;
        vec2 newLocalPosition = _transform.localPosition;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([parentEntityId, entityId, elementType, newPosition, newLocalPosition]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entityId);
                UIVisibility& visibility = uiRegistry->get<UIVisibility>(entityId);

                // Remove old parent.
                if (transform.parent)
                {
                    UITransform& oldParentTransform = uiRegistry->get<UITransform>(entt::entity(transform.parent));

                    //Remove from parents children.
                    UITransformUtils::RemoveChild(oldParentTransform, entityId);

                    visibility.parentVisible = true;
                }

                // Set new parent.
                transform.parent = entt::to_integral(parentEntityId);

                // Apply calculated new positions
                transform.position = newPosition;
                transform.localPosition = newLocalPosition;

                // Add this to parent's children.
                UITransform& parentTransform = uiRegistry->get<UITransform>(parentEntityId);
                UITransformUtils::AddChild(parentTransform, entityId, elementType);

                // Update visibility.
                UIVisibility& parentVisibility = uiRegistry->get<UIVisibility>(parentEntityId);
                visibility.parentVisible = parentVisibility.parentVisible && parentVisibility.visible;

                UpdateChildVisibility(uiRegistry, transform, visibility.parentVisible && visibility.visible);
            });
    }

    const vec2 asUITransform::GetMinBound() const
    {
        return UITransformUtils::GetMinBounds(_transform);
    }

    const vec2 asUITransform::GetMaxBound() const
    {
        return UITransformUtils::GetMaxBounds(_transform);
    }

    void asUITransform::SetVisible(bool visible)
    {
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();

        //Don't do anything if new visibility isn't different from the old one.
        if (_visibility.visible != visible)
        {
            _visibility.visible = visible;

            UpdateChildVisibilityAngelScript(uiDataSingleton, _transform, _visibility.parentVisible && _visibility.visible);
        }

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([visible, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIVisibility& uiVisibility = uiRegistry->get<UIVisibility>(entId);

                //Don't do anything if new visibility isn't different from the old one.
                if (!uiVisibility.visible != visible)
                    return;

                uiVisibility.visible = visible;

                const bool newVisibility = uiVisibility.parentVisible && uiVisibility.visible;
                UpdateChildVisibility(uiRegistry, uiRegistry->get<UITransform>(entId), newVisibility);

                if (newVisibility)
                    uiRegistry->emplace<UIVisible>(entId);
                else
                    uiRegistry->remove<UIVisible>(entId);
            });
    }

    void asUITransform::SetCollisionEnabled(bool enabled)
    {
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([enabled, entId]()
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
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([entId]()
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
            UITransform& uiChildTransform = uiRegistry->get<UITransform>(entId);

            uiChildTransform.position = UITransformUtils::GetAnchorPosition(parent, uiChildTransform.anchor);

            UpdateChildTransforms(uiRegistry, uiChildTransform);

            MarkDirty(uiRegistry, entId);
        }

        UpdateBounds(uiRegistry, parent);
    }
    void asUITransform::UpdateChildTransformsAngelScript(UI::UIDataSingleton& uiDataSingleton, UITransform& parent)
    {
        for (UIChild& child : parent.children)
        {
            auto iterator = uiDataSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (iterator == uiDataSingleton.entityToAsObject.end())
                continue;

            UITransform& asChildTransform = iterator->getSecond()->_transform;

            asChildTransform.position = UITransformUtils::GetAnchorPosition(parent, asChildTransform.anchor);

            UpdateChildTransformsAngelScript(uiDataSingleton, asChildTransform);
        }
    }

    void asUITransform::UpdateChildVisibility(entt::registry* uiRegistry, const UITransform& parent, bool parentVisibility)
    {
        for (const UIChild& child : parent.children)
        {
            const entt::entity childEntity = entt::entity(child.entity);
            UIVisibility& uiChildVisibility = uiRegistry->get<UIVisibility>(childEntity);

            const bool visibilityChanged = uiChildVisibility.parentVisible != parentVisibility;
            if (!visibilityChanged)
                continue;

            uiChildVisibility.parentVisible = parentVisibility;

            const bool newVisibility = uiChildVisibility.parentVisible && uiChildVisibility.visible;
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
            UIVisibility& uiChildVisibility = asChild->_visibility;

            const bool visibilityChanged = uiChildVisibility.parentVisible != parentVisibility;
            if (!visibilityChanged)
                continue;

            uiChildVisibility.parentVisible = parentVisibility;

            const bool newVisibility = uiChildVisibility.parentVisible && uiChildVisibility.visible;
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

        UpdateParentBounds(uiRegistry, uiRegistry->get<UITransform>(entt::entity(transform.parent)), transform.minBound, transform.maxBound);
    }
    void asUITransform::UpdateParentBounds(entt::registry* uiRegistry, UITransform& parent, vec2 childMin, vec2 childMax)
    {
        if (!parent.includeChildBounds)
            return;

        if (childMin.x < parent.minBound.x) parent.minBound.x = childMin.x;
        if (childMin.y < parent.minBound.y) parent.minBound.y = childMin.y;

        if (childMax.x > parent.maxBound.x) parent.maxBound.x = childMax.x;
        if (childMax.y > parent.maxBound.y) parent.maxBound.y = childMax.y;

        if (parent.parent)
            UpdateParentBounds(uiRegistry, uiRegistry->get<UITransform>(entt::entity(parent.parent)), parent.minBound, parent.maxBound);
    }
}
