#include "asUITransform.h"
#include "../../../Utils/ServiceLocator.h"

#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/Singletons/UIDataSingleton.h"
#include "../../../ECS/Components/UI/Singletons/UIAddElementQueueSingleton.h"

#include "../../../ECS/Components/UI/UIVisible.h"
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
        UpdateChildPositionsInAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([position, hasParent, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                if (!uiRegistry->has<UIDirty>(entId))
                    uiRegistry->emplace<UIDirty>(entId);
                if (hasParent)
                    uiTransform.localPosition = position;
                else
                    uiTransform.position = position;

                UpdateChildPositions(uiRegistry, uiTransform);
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

        UpdateChildPositionsInAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([anchor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.anchor = anchor;

                if (uiTransform.parent)
                {
                    UITransform& parent = uiRegistry->get<UITransform>(entId);

                    uiTransform.position = UITransformUtils::GetAnchorPosition(parent, uiTransform.anchor);
                }

                UpdateChildPositions(uiRegistry, uiTransform);
                
                if (!uiRegistry->has<UIDirty>(entId))
                    uiRegistry->emplace<UIDirty>(entId);
            });
    }

    void asUITransform::SetLocalAnchor(const vec2& localAnchor)
    {
        _transform.localAnchor = localAnchor;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();
        UpdateChildPositionsInAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([localAnchor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.localAnchor = localAnchor;

                UpdateChildPositions(uiRegistry, uiTransform);

                if (!uiRegistry->has<UIDirty>(entId))
                    uiRegistry->emplace<UIDirty>(entId);
            });
    }

    void asUITransform::SetSize(const vec2& size)
    {
        _transform.size = size;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();
        UpdateChildPositionsInAngelScript(uiDataSingleton, _transform);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([size, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.size = size;

                UpdateChildPositions(uiRegistry, uiTransform);

                if (!uiRegistry->has<UIDirty>(entId))
                    uiRegistry->emplace<UIDirty>(entId);
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
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.sortData.depth = depth;

                if (!uiRegistry->has<UIDirty>(entId))
                    uiRegistry->emplace<UIDirty>(entId);
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

        // Update visiblity
        UIVisiblity& parentVisibility = parent->_visibility;
        _visibility.parentVisible = parentVisibility.parentVisible && parentVisibility.visible;

        UpdateChildVisiblityInAngelScript(uiDataSingleton, _transform, _visibility.parentVisible && _visibility.visible);

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
                UIVisiblity& visiblity = uiRegistry->get<UIVisiblity>(entityId);

                // Remove old parent.
                if (transform.parent)
                {
                    UITransform& oldParentTransform = uiRegistry->get<UITransform>(entt::entity(transform.parent));

                    //Remove from parents children.
                    UITransformUtils::RemoveChild(oldParentTransform, entityId);

                    visiblity.parentVisible = true;
                }

                // Set new parent.
                transform.parent = entt::to_integral(parentEntityId);

                // Apply calculated new positions
                transform.position = newPosition;
                transform.localPosition = newLocalPosition;

                // Add this to parent's children.
                UITransform& parentTransform = uiRegistry->get<UITransform>(parentEntityId);
                UITransformUtils::AddChild(parentTransform, entityId, elementType);

                // Update visiblity.
                UIVisiblity& parentVisibility = uiRegistry->get<UIVisiblity>(parentEntityId);
                visiblity.parentVisible = parentVisibility.parentVisible && parentVisibility.visible;

                UpdateChildVisiblity(uiRegistry, transform, visiblity.parentVisible && visiblity.visible);
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

        _visibility.visible = visible;

        UpdateChildVisiblityInAngelScript(uiDataSingleton, _transform, _visibility.parentVisible && _visibility.visible);

        // TRANSACTION
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([visible, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIVisiblity& uiVisibility = uiRegistry->get<UIVisiblity>(entId);

                const bool visiblityChanged = uiVisibility.visible != visible;
                if (!visiblityChanged)
                    return;

                uiVisibility.visible = visible;

                const bool newVisiblity = uiVisibility.parentVisible && uiVisibility.visible;
                UpdateChildVisiblity(uiRegistry, uiRegistry->get<UITransform>(entId), newVisiblity);

                if (newVisiblity)
                    uiRegistry->emplace<UIVisible>(entId);
                else
                    uiRegistry->remove<UIVisible>(entId);
            });
    }

    void asUITransform::MarkDirty(entt::registry* uiRegistry, entt::entity entId)
    {
        if (!uiRegistry->has<UIDirty>(entId))
            uiRegistry->emplace<UIDirty>(entId);
    }

    void asUITransform::UpdateChildPositions(entt::registry* uiRegistry, UITransform& parent)
    {
        for (UIChild& child : parent.children)
        {
            entt::entity entId = entt::entity(child.entity);
            UITransform& uiChildTransform = uiRegistry->get<UITransform>(entId);

            uiChildTransform.position = UITransformUtils::GetAnchorPosition(parent, uiChildTransform.anchor);

            UpdateChildPositions(uiRegistry, uiChildTransform);

            MarkDirty(uiRegistry, entId);
        }
    }

    void asUITransform::UpdateChildPositionsInAngelScript(UI::UIDataSingleton& uiDataSingleton, UITransform& parent)
    {
        for (UIChild& child : parent.children)
        {
            auto iterator = uiDataSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (iterator == uiDataSingleton.entityToAsObject.end())
                continue;

            UITransform& asChildTransform = iterator->getSecond()->_transform;

            asChildTransform.position = UITransformUtils::GetAnchorPosition(parent, asChildTransform.anchor);

            UpdateChildPositionsInAngelScript(uiDataSingleton, asChildTransform);
        }
    }

    void asUITransform::UpdateChildVisiblity(entt::registry* uiRegistry, const UITransform& parent, bool parentVisiblity)
    {
        for (const UIChild& child : parent.children)
        {
            const entt::entity childEntity = entt::entity(child.entity);
            UIVisiblity& uiChildVisiblity = uiRegistry->get<UIVisiblity>(childEntity);

            const bool visiblityChanged = uiChildVisiblity.parentVisible != parentVisiblity;
            if (!visiblityChanged)
                continue;

            uiChildVisiblity.parentVisible = parentVisiblity;

            const bool newVisiblity = uiChildVisiblity.parentVisible && uiChildVisiblity.visible;
            UpdateChildVisiblity(uiRegistry, uiRegistry->get<UITransform>(childEntity), newVisiblity);

            if (newVisiblity)
                uiRegistry->emplace<UIVisible>(entt::entity(child.entity));
            else
                uiRegistry->remove<UIVisible>(entt::entity(child.entity));
        }
    }

    void asUITransform::UpdateChildVisiblityInAngelScript(UI::UIDataSingleton& uiDataSingleton, const UITransform& parent, bool parentVisiblity)
    {
        for (const UIChild& child : parent.children)
        {
            auto iterator = uiDataSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (iterator == uiDataSingleton.entityToAsObject.end())
                continue;

            asUITransform* asChild = iterator->getSecond();
            UIVisiblity& uiChildVisiblity = asChild->_visibility;

            const bool visiblityChanged = uiChildVisiblity.parentVisible != parentVisiblity;
            if (!visiblityChanged)
                continue;

            uiChildVisiblity.parentVisible = parentVisiblity;

            const bool newVisiblity = uiChildVisiblity.parentVisible && uiChildVisiblity.visible;
            UpdateChildVisiblityInAngelScript(uiDataSingleton, asChild->_transform, newVisiblity);
        }
    }
}
