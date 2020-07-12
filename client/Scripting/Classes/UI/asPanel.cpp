#include "asPanel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"

#include "../../../ECS/Components/UI/Singletons/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"

namespace UI
{
    asPanel::asPanel(entt::entity entityId) : asUITransform(entityId, UIElementType::UITYPE_PANEL) { }

    void asPanel::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Panel", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<asUITransform, asPanel>("UITransform");
        r = ScriptEngine::RegisterScriptFunction("Panel@ CreatePanel()", asFUNCTION(asPanel::CreatePanel)); assert(r >= 0);

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

    void asPanel::SetOnClickCallback(asIScriptFunction* callback)
    {
        _events.onClickCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
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
        _events.onDraggedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
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
        _events.onFocusedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
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
        _image.texture = texture;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([texture, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIImage& image = uiRegistry->get<UIImage>(entId);

                image.texture = texture;
                MarkDirty(uiRegistry, entId);
            });
    }
    void asPanel::SetColor(const Color& color)
    {
        _image.color = color;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([color, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIImage& image = uiRegistry->get<UIImage>(entId);

                image.color = color;
                MarkDirty(uiRegistry, entId);
            });
    }

    asPanel* asPanel::CreatePanel()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIEntityPoolSingleton& entityPool = registry->ctx<UIEntityPoolSingleton>();

        asPanel* panel = new asPanel(entityPool.GetId());
        
        return panel;
    }
}