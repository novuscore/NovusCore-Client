#include "asCheckbox.h"
#include "asPanel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../UI/ImageUtils.h"
#include "../../../UI/TransformEventUtils.h"

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

        UI::TransformEventUtils::SetOnClickCallback(callback, _entityId);
    }

    void asCheckbox::SetOnDragCallback(asIScriptFunction* callback)
    {
        _events.onDraggedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);

        UI::TransformEventUtils::SetOnDragCallback(callback, _entityId);
    }

    void asCheckbox::SetOnFocusCallback(asIScriptFunction* callback)
    {
        _events.onFocusedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        UI::TransformEventUtils::SetOnFocusCallback(callback, _entityId);
    }

    void asCheckbox::SetBackgroundTexture(const std::string& texture)
    {
        _image.texture = texture;

        UI::ImageUtils::SetTexture(texture, _entityId);
    }
    void asCheckbox::SetBackgroundColor(const Color& color)
    {
        _image.color = color;

        UI::ImageUtils::SetColor(color, _entityId);
    }

    void asCheckbox::SetCheckTexture(const std::string& texture)
    {
        checkPanel->SetTexture(texture);
    }

    const std::string& asCheckbox::GetCheckTexture() const
    {
        return checkPanel->GetTexture();
    }

    void asCheckbox::SetCheckColor(const Color& color)
    {
        checkPanel->SetColor(color);
    }

    const Color asCheckbox::GetCheckColor() const
    {
        return checkPanel->GetColor();
    }

    void asCheckbox::SetChecked(bool checked)
    {
        _checkBox.checked = checked;

        checkPanel->SetVisible(checked);

        entt::entity entId = _entityId;
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([checked, entId]()
            {
                UICheckbox checkBox = ServiceLocator::GetUIRegistry()->get<UICheckbox>(entId);
                checkBox.checked = checked;
            });
    }

    asCheckbox* asCheckbox::CreateCheckbox()
    {
        UIEntityPoolSingleton& entityPool = ServiceLocator::GetUIRegistry()->ctx<UIEntityPoolSingleton>();

        asCheckbox* checkbox = new asCheckbox(entityPool.GetId());
        
        return checkbox;
    }
}