#include "asUITransform.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"

namespace UI
{

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

    void asUITransform::SetDepth(const u16& depth)
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

    void asUITransform::UpdateChildren(entt::registry* registry, UITransform& transform, vec2 position)
    {
        if(!transform.children.size())
            return;

        for (UITransform::UIChild& child : transform.children)
        {
            UITransform& uiChildTransform = registry->get<UITransform>(entt::entity(child.entity));
            uiChildTransform.position = position;

            if (uiChildTransform.children.size())
            {
                UpdateChildren(registry, uiChildTransform, uiChildTransform.position + uiChildTransform.localPosition);
            }
        }
    }
}
