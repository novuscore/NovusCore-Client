#include "asUITransform.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/UIDataSingleton.h"

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

                entt::entity entId = _entityId;
                auto position = std::find_if(oldParentTransform.children.begin(), oldParentTransform.children.end(), [entId](UITransform::UIChild& uiChild)
                    {
                        return uiChild.entity == entt::to_integral(entId);
                    });

                if (position != oldParentTransform.children.end())
                    oldParentTransform.children.erase(position);
            }

            //Keep same absolute position.
            _transform.position = _transform.position + _transform.localPosition;
            _transform.localPosition = vec2(0, 0);
        }

        // Set new parent.
        _transform.parent = entt::to_integral(parent->_entityId);

        UITransform& newParentTransform = parent->_transform;

        //Recalculate new local positon whilst keeping absolute position.
        _transform.localPosition = (newParentTransform.position + newParentTransform.localPosition) - _transform.position;
        _transform.position = newParentTransform.position + newParentTransform.localPosition;

        //Add ourselves to parents angelscript object children
        UITransform::UIChild thisChild;
        thisChild.entity = entt::to_integral(_entityId);
        thisChild.type = _elementType;
        parent->_transform.children.push_back(thisChild);

        // Transaction.
        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        UIElementData::UIElementType elementType = _elementType;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([parent, entId, elementType]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);

                //Remove old parent.
                if (transform.parent)
                {
                    UITransform& oldParentTransform = uiRegistry->get<UITransform>(entt::entity(transform.parent));

                    //Remove from parents children.
                    auto position = std::find_if(oldParentTransform.children.begin(), oldParentTransform.children.end(), [entId](UITransform::UIChild& uiChild)
                        {
                            return uiChild.entity == entt::to_integral(entId);
                        });

                    if (position != oldParentTransform.children.end())
                        oldParentTransform.children.erase(position);

                    //Keep same absolute position.
                    transform.position = transform.position + transform.localPosition;
                    transform.localPosition = vec2(0, 0);
                }

                //Set new parent.
                transform.parent = entt::to_integral(parent->GetEntityId());

                UITransform& newParentTransform = uiRegistry->get<UITransform>(entt::entity(parent->GetEntityId()));

                //Recalculate new local positon whilst keeping absolute position.
                transform.localPosition = (newParentTransform.position + newParentTransform.localPosition) - transform.position;
                transform.position = newParentTransform.position + newParentTransform.localPosition;

                //Add this to parent's children.
                UITransform::UIChild thisAsChild;
                thisAsChild.entity = entt::to_integral(entId);
                thisAsChild.type = elementType;

                newParentTransform.children.push_back(thisAsChild);
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
