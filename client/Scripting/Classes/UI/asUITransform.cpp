#include "asUITransform.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/UIDataSingleton.h"
#include "../../../ECS/Components/UI/UITransformUtils.h"

namespace UI
{
    asUITransform::asUITransform(entt::entity entityId, UIElementData::UIElementType elementType) : _entityId(entityId), _elementType(elementType)
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
        UpdateChildrenPositionInAngelScript(uiDataSingleton, _transform, position);

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

                if (uiTransform.children.size())
                {
                    UpdateChildrenPosition(uiRegistry, uiTransform, position);
                }
            });
    }

    void asUITransform::SetLocalAnchor(const vec2& localAnchor)
    {
        _transform.localAnchor = localAnchor;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([localAnchor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.localAnchor = localAnchor;
                uiTransform.isDirty = true;
            });
    }

    void asUITransform::SetSize(const vec2& size)
    {
        _transform.size = size;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([size, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.size = size;
                uiTransform.isDirty = true;
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
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();

        // Remove old parent.
        if (_transform.parent)
        {
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

        UITransform& newParentTransform = parent->_transform;

        //Recalculate new local position whilst keeping absolute position.
        _transform.localPosition = (newParentTransform.position + newParentTransform.localPosition) - _transform.position;
        _transform.position = newParentTransform.position + newParentTransform.localPosition;

        //Add ourselves to parents angelscript object children
        UITransformUtils::AddChild(newParentTransform, _entityId, _elementType);

        // Transaction.
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entityId = _entityId;
        UIElementData::UIElementType elementType = _elementType;
        vec2 newPosition = _transform.position;
        vec2 newLocalPosition = _transform.localPosition;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([parent, entityId, elementType, newPosition, newLocalPosition]()
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
                transform.parent = entt::to_integral(parent->GetEntityId());

                UITransform& newParentTransform = uiRegistry->get<UITransform>(entt::entity(parent->GetEntityId()));

                //Apply calculated new positions
                transform.position = newPosition;
                transform.localPosition = newLocalPosition;

                //Add this to parent's children.
                UITransformUtils::AddChild(newParentTransform, entityId, elementType);
            });
        //Transaction End.
    }

    void asUITransform::UpdateChildrenPosition(entt::registry* uiRegistry, UITransform& parent, vec2 position)
    {
        for (UITransform::UIChild& child : parent.children)
        {
            UITransform& uiChildTransform = uiRegistry->get<UITransform>(entt::entity(child.entity));
            uiChildTransform.position = position;
            uiChildTransform.isDirty = true;

            if (uiChildTransform.children.size())
            {
                UpdateChildrenPosition(uiRegistry, uiChildTransform, uiChildTransform.position + uiChildTransform.localPosition);
            }
        }
    }
    void asUITransform::UpdateChildrenPositionInAngelScript(UI::UIDataSingleton& uiDataSingleton, UITransform& parent, vec2 position)
    {
        for (UITransform::UIChild& child : parent.children)
        {
            auto iterator = uiDataSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (iterator != uiDataSingleton.entityToAsObject.end())
            {
                asUITransform* asChild = iterator->getSecond();
                asChild->_transform.position = position;

                if (asChild->_transform.children.size())
                {
                    UpdateChildrenPositionInAngelScript(uiDataSingleton, asChild->_transform, asChild->_transform.position + asChild->_transform.localPosition);
                }
            }

        }
    }
}
