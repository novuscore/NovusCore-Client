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
                    UpdateChildren(uiRegistry, uiTransform, position);
                }
            });
    }

    void asUITransform::SetAnchor(const vec2& anchor)
    {
        _transform.anchor = anchor;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([anchor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.isDirty = true;
                uiTransform.anchor = anchor;
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

                uiTransform.isDirty = true;
                uiTransform.size = size;
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

                uiTransform.isDirty = true;
                uiTransform.depth = depth;
            });
    }

    void asUITransform::SetParent(asUITransform* parent)
    {
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();

        // Remove old parent.
        if (_transform.parent)
        {
            UITransform& parentTransform = uiRegistry->get<UITransform>(entt::entity(_transform.parent));

            //Keep same absolute position.
            _transform.position = (parentTransform.position + parentTransform.localPosition) - (_transform.position + _transform.localPosition);
            _transform.localPosition = vec2(0, 0);
        }

        // Set new parent.
        _transform.parent = entt::to_integer(parent->_entityId);

        UITransform& newParentTransform = parent->_transform;
        // Recalculate new position.
        _transform.localPosition = newParentTransform.position + newParentTransform.localPosition - _transform.position;
        _transform.position = newParentTransform.position + newParentTransform.localPosition;


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
                    UITransform& parentTransform = uiRegistry->get<UITransform>(entt::entity(transform.parent));

                    //Remove from parents children.
                    auto position = parentTransform.children.begin();
                    for (position; position < parentTransform.children.end(); position++)
                    {
                        if (position->entity == entt::to_integer(entId))
                            break;
                    }
                    if (position != parentTransform.children.end())
                        parentTransform.children.erase(position);

                    //Keep same absolute position.
                    transform.position = (parentTransform.position + parentTransform.localPosition) - (transform.position + transform.localPosition);
                    transform.localPosition = vec2(0, 0);
                }

                //Set new parent.
                transform.parent = entt::to_integer(parent->GetEntityId());

                UITransform& newParentTransform = uiRegistry->get<UITransform>(entt::entity(parent->GetEntityId()));

                //Recalculate new positon.
                transform.localPosition = newParentTransform.position + newParentTransform.localPosition - transform.position;
                transform.position = newParentTransform.position + newParentTransform.localPosition;

                //Add this to parent's children.
                UITransform::UIChild thisChild;
                thisChild.entity = entt::to_integer(entId);
                thisChild.type = elementType;

                newParentTransform.children.push_back(thisChild);
            });
    }

    void asUITransform::UpdateChildren(entt::registry* registry, UITransform& transform, vec2 position)
    {
        if(!transform.children.size())
            return;

        UIDataSingleton& uiSingleton = registry->ctx<UIDataSingleton>();

        for (UITransform::UIChild& child : transform.children)
        {
            UITransform& uiChildTransform = registry->get<UITransform>(entt::entity(child.entity));
            uiChildTransform.position = position;
            uiChildTransform.isDirty = true;

            //Apply position updates to asObject.
            auto iterator = uiSingleton.entityToAsObject.find(entt::entity(child.entity));
            if (iterator != uiSingleton.entityToAsObject.end())
            {
                iterator->second->_transform.position = position;
            }

            if (uiChildTransform.children.size())
            {
                UpdateChildren(registry, uiChildTransform, uiChildTransform.position + uiChildTransform.localPosition);
            }
        }
    }
}
