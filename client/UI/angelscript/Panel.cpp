#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Collidable.h"

namespace UIScripting
{
    Panel::Panel() : BaseElement(UI::UIElementType::UITYPE_PANEL)
    { 
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UISingleton::UILockSingleton& uiLockSingleton = registry->ctx<UISingleton::UILockSingleton>();
        uiLockSingleton.mutex.lock();
        {
            _transform = &registry->emplace<UIComponent::Transform>(_entityId);
            _transform->sortData.entId = _entityId;
            _transform->sortData.type = _elementType;
            _transform->asObject = this;

            registry->emplace<UIComponent::Visible>(_entityId);
            _visibility = &registry->emplace<UIComponent::Visibility>(_entityId);
            _image = &registry->emplace<UIComponent::Image>(_entityId);
            registry->emplace<UIComponent::Renderable>(_entityId);
            registry->emplace<UIComponent::Collidable>(_entityId);

            _events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
            _events->asObject = this;
        }
        uiLockSingleton.mutex.unlock();
    }

    void Panel::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Panel", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Panel>("Transform");
        r = ScriptEngine::RegisterScriptFunction("Panel@ CreatePanel()", asFUNCTION(Panel::CreatePanel)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetEventFlag(int8 flags)", asMETHOD(Panel, SetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void UnsetEventFlag(int8 flags)", asMETHOD(Panel, UnsetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(Panel, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsDraggable()", asMETHOD(Panel, IsDraggable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(Panel, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void PanelEventCallback(Panel@ panel)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnClickCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnDragged(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnDragCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocused(PanelEventCallback@ cb)", asMETHOD(Panel, SetOnFocusCallback)); assert(r >= 0);

        // Renderable Functions
        r = ScriptEngine::RegisterScriptClassFunction("string GetTexture()", asMETHOD(Panel, GetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string Texture)", asMETHOD(Panel, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(Panel, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Panel, SetColor)); assert(r >= 0);
    }

    void Panel::SetOnClickCallback(asIScriptFunction* callback)
    {
        _events->onClickCallback = callback;
        _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }

    void Panel::SetOnDragCallback(asIScriptFunction* callback)
    {
        _events->onDraggedCallback = callback;
        _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
    }

    void Panel::SetOnFocusCallback(asIScriptFunction* callback)
    {
        _events->onFocusedCallback = callback;
        _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
    }

    void Panel::SetTexture(const std::string& texture)
    {
        _image->texture = texture;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }
    void Panel::SetColor(const Color& color)
    {
        _image->color = color;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    Panel* Panel::CreatePanel()
    {
        Panel* panel = new Panel();
        
        return panel;
    }
}