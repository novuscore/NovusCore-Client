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

        // Transform Functions
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

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetEventFlag(int8 flags)", asMETHOD(asPanel, SetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void UnsetEventFlag(int8 flags)", asMETHOD(asPanel, UnsetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(asPanel, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsDraggable()", asMETHOD(asPanel, IsDraggable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(asPanel, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void PanelEventCallback(Panel@ panel)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(PanelEventCallback@ cb)", asMETHOD(asPanel, SetOnClickCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnDragged(PanelEventCallback@ cb)", asMETHOD(asPanel, SetOnDragCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocused(PanelEventCallback@ cb)", asMETHOD(asPanel, SetOnFocusCallback)); assert(r >= 0);

        // Renderable Functions
        r = ScriptEngine::RegisterScriptClassFunction("string GetTexture()", asMETHOD(asPanel, GetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string Texture)", asMETHOD(asPanel, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(asPanel, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(asPanel, SetColor)); assert(r >= 0);
    }

    void asPanel::SetPosition(const vec2& position)
    {
        transform.position = position;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([position, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);

                transform.isDirty = true;
                transform.position = position;
            });
    }
    void asPanel::SetLocalPosition(const vec2& localPosition)
    {
        transform.localPosition = localPosition;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([localPosition, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);

                transform.isDirty = true;
                transform.localPosition = localPosition;
            });
    }
    void asPanel::SetAnchor(const vec2& anchor)
    {
        transform.anchor = anchor;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([anchor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);

                transform.isDirty = true;
                transform.anchor = anchor;
            });
    }
    void asPanel::SetSize(const vec2& size)
    {
        transform.size = size;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([size, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);

                transform.isDirty = true;
                transform.size = size;
            });
    }
    void asPanel::SetDepth(const u16& depth)
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

    void asPanel::SetOnClickCallback(asIScriptFunction* callback)
    {
        events.onClickCallback = callback;
        events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);
                events.onClickCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
            });
    }

    void asPanel::SetOnDragCallback(asIScriptFunction* callback)
    {
        events.onDraggedCallback = callback;
        events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);
                events.onDraggedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
            });
    }

    void asPanel::SetOnFocusCallback(asIScriptFunction* callback)
    {
        events.onFocusedCallback = callback;
        events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);
                events.onFocusedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });
    }

    void asPanel::SetTexture(const std::string& texture)
    {
        renderable.texture = texture;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([texture, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);
                UIRenderable& renderable = uiRegistry->get<UIRenderable>(entId);

                transform.isDirty = true;
                renderable.texture = texture;
            });
    }
    void asPanel::SetColor(const Color& color)
    {
        renderable.color = color;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([color, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& transform = uiRegistry->get<UITransform>(entId);
                UIRenderable& renderable = uiRegistry->get<UIRenderable>(entId);

                transform.isDirty = true;
                renderable.color = color;
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