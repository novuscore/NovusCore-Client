#include "asCheckbox.h"
#include "asPanel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"

#include "../../../ECS/Components/UI/Singletons/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"


namespace UI
{
    asCheckbox::asCheckbox(entt::entity entityId) : asUITransform(entityId, UIElementType::UITYPE_CHECKBOX)
    {
        checkPanel = asPanel::CreatePanel();
        checkPanel->SetFillParentSize(true);
        checkPanel->SetParent(this);
    }

    void asCheckbox::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Checkbox", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<asUITransform, asCheckbox>("UITransform");
        r = ScriptEngine::RegisterScriptFunction("Checkbox@ CreateCheckbox()", asFUNCTION(asCheckbox::CreateCheckbox)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetEventFlag(int8 flags)", asMETHOD(asCheckbox, SetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void UnsetEventFlag(int8 flags)", asMETHOD(asCheckbox, UnsetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(asCheckbox, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsDraggable()", asMETHOD(asCheckbox, IsDraggable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(asCheckbox, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void CheckboxEventCallback(Checkbox@ checkbox)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(CheckboxEventCallback@ cb)", asMETHOD(asCheckbox, SetOnClickCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnDragged(CheckboxEventCallback@ cb)", asMETHOD(asCheckbox, SetOnDragCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocused(CheckboxEventCallback@ cb)", asMETHOD(asCheckbox, SetOnFocusCallback)); assert(r >= 0);

        // Renderable Functions
        r = ScriptEngine::RegisterScriptClassFunction("string GetBackgroundTexture()", asMETHOD(asCheckbox, GetBackgroundTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBackgroundTexture(string Texture)", asMETHOD(asCheckbox, SetBackgroundTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetBackgroundColor()", asMETHOD(asCheckbox, GetBackgroundColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBackgroundColor(Color color)", asMETHOD(asCheckbox, SetBackgroundColor)); assert(r >= 0);
    }

    void asCheckbox::SetOnClickCallback(asIScriptFunction* callback)
    {
        _events.onClickCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);

        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);

                events.onClickCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
            });
    }

    void asCheckbox::SetOnDragCallback(asIScriptFunction* callback)
    {
        _events.onDraggedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);

        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);

                events.onDraggedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
            });
    }

    void asCheckbox::SetOnFocusCallback(asIScriptFunction* callback)
    {
        _events.onFocusedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);

                events.onFocusedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });
    }

    void asCheckbox::SetBackgroundTexture(const std::string& texture)
    {
        _image.texture = texture;

        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([texture, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIImage& image = uiRegistry->get<UIImage>(entId);

                image.texture = texture;
                MarkDirty(uiRegistry, entId);
            });
    }
    void asCheckbox::SetBackgroundColor(const Color& color)
    {
        _image.color = color;

        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([color, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIImage& image = uiRegistry->get<UIImage>(entId);

                image.color = color;
                MarkDirty(uiRegistry, entId);
            });
    }

    asCheckbox* asCheckbox::CreateCheckbox()
    {
        UIEntityPoolSingleton& entityPool = ServiceLocator::GetUIRegistry()->ctx<UIEntityPoolSingleton>();

        asCheckbox* checkbox = new asCheckbox(entityPool.GetId());
        
        return checkbox;
    }
}