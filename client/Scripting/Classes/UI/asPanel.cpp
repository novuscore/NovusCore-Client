#include "asPanel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/UI/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/UIAddElementQueueSingleton.h"

namespace UI
{
    void asPanel::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Panel", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptFunction("Panel@ CreatePanel()", asFUNCTION(asPanel::CreatePanel)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetPosition()", asMETHOD(asPanel, GetPosition)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetPosition(vec2 pos)", asMETHOD(asPanel, SetPosition)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetLocalPosition()", asMETHOD(asPanel, GetLocalPosition)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetLocalPosition(vec2 localPosition)", asMETHOD(asPanel, SetLocalPosition)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetAnchor()", asMETHOD(asPanel, GetAnchor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetAnchor(vec2 anchor)", asMETHOD(asPanel, SetAnchor)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetSize()", asMETHOD(asPanel, GetSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetSize(vec2 size)", asMETHOD(asPanel, SetSize)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("float GetDepth()", asMETHOD(asPanel, GetDepth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetDepth(float depth)", asMETHOD(asPanel, SetDepth)); assert(r >= 0);
    }

    vec2 asPanel::GetPosition()
    {
        return transform.position;
    }
    void asPanel::SetPosition(vec2& position)
    {
        transform.position = position;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([position, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);
                transform.position = position;
            });
    }
    vec2 asPanel::GetLocalPosition()
    {
        return transform.localPosition;
    }
    void asPanel::SetLocalPosition(vec2& localPosition)
    {
        transform.localPosition = localPosition;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([localPosition, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);
                transform.localPosition = localPosition;
            });
    }
    vec2 asPanel::GetAnchor()
    {
        return transform.anchor;
    }
    void asPanel::SetAnchor(vec2& anchor)
    {
        transform.anchor = anchor;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([anchor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);
                transform.anchor = anchor;
            });
    }
    vec2 asPanel::GetSize()
    {
        return transform.size;
    }
    void asPanel::SetSize(vec2& size)
    {
        transform.size = size;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([size, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);
                transform.size = size;
            });
    }
    u16 asPanel::GetDepth()
    {
        return transform.depth;
    }
    void asPanel::SetDepth(u16& depth)
    {
        transform.depth = depth;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([depth, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);
                transform.depth = depth;
            });
    }
    asPanel* asPanel::CreatePanel()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIEntityPoolSingleton& entityPool = registry->ctx<UIEntityPoolSingleton>();
        UIAddElementQueueSingleton& addElementQueue = registry->ctx<UIAddElementQueueSingleton>();

        UIElementData elementData;
        entityPool.entityIdPool.try_dequeue(elementData.entityId);
        elementData.type = UIElementData::UIElementType::UITYPE_PANEL;

        asPanel* panel = new asPanel();
        panel->entityId = elementData.entityId;
        addElementQueue.elementPool.enqueue(elementData);

        return panel;
    }
}