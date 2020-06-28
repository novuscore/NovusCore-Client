#include "asUITransform.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/UIDataSingleton.h"
#include "../../../ECS/Components/UI/UITransformUtils.h"

namespace UI
{
    asUITransform::asUITransform(entt::entity entityId, UIElementType elementType) : _entityId(entityId), _elementType(elementType)
    {
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();
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

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([position, hasParent, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.isDirty = true;      
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

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([anchor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.anchor = anchor;
                uiTransform.isDirty = true;

                if (uiTransform.parent)
                {
                    UITransform& parent = uiRegistry->get<UITransform>(entId);

                    uiTransform.position = UITransformUtils::GetAnchorPosition(parent, uiTransform.anchor);
                }

                UpdateChildPositions(uiRegistry, uiTransform);
            });
    }

    void asUITransform::SetLocalAnchor(const vec2& localAnchor)
    {
        _transform.localAnchor = localAnchor;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();
        UpdateChildPositionsInAngelScript(uiDataSingleton, _transform);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([localAnchor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.localAnchor = localAnchor;
                uiTransform.isDirty = true;
                
                UpdateChildPositions(uiRegistry, uiTransform);
            });
    }

    void asUITransform::SetSize(const vec2& size)
    {
        _transform.size = size;

        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();
        UpdateChildPositionsInAngelScript(uiDataSingleton, _transform);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([size, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.size = size;
                uiTransform.isDirty = true;
                
                UpdateChildPositions(uiRegistry, uiTransform);
            });
    }

    void asUITransform::SetDepth(const u16 depth)
    {
        _transform.depth = depth;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([depth, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.depth = depth;
                uiTransform.isDirty = true;
            });
    }

    void asUITransform::SetParent(asUITransform* parent)
    {
        // Remove old parent.
        if (_transform.parent)
        {
            entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
            UIDataSingleton& uiDataSingleton = uiRegistry->ctx<UIDataSingleton>();

            //Find parent transform as object.
            auto entityToAsObjectIterator = uiDataSingleton.entityToAsObject.find(entt::entity(_transform.parent));
            if (entityToAsObjectIterator != uiDataSingleton.entityToAsObject.end())
            {
                UITransform& oldParentTransform = entityToAsObjectIterator->getSecond()->_transform;

                UITransformUtils::RemoveChild(oldParentTransform, _entityId);
            }

            //Keep same absolute position.
            _transform.position = _transform.position + _transform.localPosition;
            _transform.localPosition = vec2(0, 0);
        }

        // Set new parent.
        _transform.parent = entt::to_integral(parent->_entityId);

        //Calculate origin.
        UITransform& newParentTransform = parent->_transform;
        vec2 NewOrigin = UITransformUtils::GetAnchorPosition(newParentTransform, _transform.anchor);

        //Recalculate new local position whilst keeping absolute position.
        _transform.localPosition = (NewOrigin + newParentTransform.localPosition) - _transform.position;
        _transform.position = NewOrigin + newParentTransform.localPosition;

        //Add ourselves to parent's angelscript object children
        UITransformUtils::AddChild(newParentTransform, _entityId, _elementType);

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

                //Remove old parent.
                if (transform.parent)
                {
                    UITransform& oldParentTransform = uiRegistry->get<UITransform>(entt::entity(transform.parent));

                    //Remove from parents children.
                    UITransformUtils::RemoveChild(oldParentTransform, entityId);
                }

                //Set new parent.
                transform.parent = entt::to_integral(parentEntityId);

                //Apply calculated new positions
                transform.position = newPosition;
                transform.localPosition = newLocalPosition;

                //Add this to parent's children.
                UITransform& newParentTransform = uiRegistry->get<UITransform>(parentEntityId);
                UITransformUtils::AddChild(newParentTransform, entityId, elementType);
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

    void asUITransform::UpdateChildPositions(entt::registry* uiRegistry, UITransform& parent)
    {
        for (UITransform::UIChild& child : parent.children)
        {
            UITransform& uiChildTransform = uiRegistry->get<UITransform>(entt::entity(child.entity));

            uiChildTransform.position = UITransformUtils::GetAnchorPosition(parent, uiChildTransform.anchor);
            uiChildTransform.isDirty = true;

            UpdateChildPositions(uiRegistry, uiChildTransform);
        }
    }

    void asUITransform::UpdateChildPositionsInAngelScript(UI::UIDataSingleton& uiDataSingleton, UITransform& parent)
    {
        for (UITransform::UIChild& child : parent.children)
        {
            auto iterator = uiDataSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (iterator != uiDataSingleton.entityToAsObject.end())
            {
                asUITransform* asChild = iterator->getSecond();

                asChild->_transform.position = UITransformUtils::GetAnchorPosition(parent, asChild->_transform.anchor);

                if (asChild->_transform.children.size())
                {
                    UpdateChildPositionsInAngelScript(uiDataSingleton, asChild->_transform);
                }
            }

        }
    }
}
