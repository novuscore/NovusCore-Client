#include "Button.h"
#include "Label.h"
#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Collidable.h"

namespace UIScripting
{
    Button::Button() : BaseElement(UI::UIElementType::UITYPE_BUTTON) 
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
            registry->emplace<UIComponent::Collidable>(_entityId);

            _events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
            _events->asObject = this;
        }
        uiLockSingleton.mutex.unlock();

        _panel = Panel::CreatePanel();
        _panel->SetFillParentSize(true);
        _panel->SetAnchor(vec2(0.5, 0.5));
        _panel->SetLocalAnchor(vec2(0.5, 0.5));
        _panel->SetParent(this);
        _panel->SetCollisionEnabled(false);

        _label = Label::CreateLabel();
        _label->SetFillParentSize(true);
        _label->SetAnchor(vec2(0.5, 0.5));
        _label->SetLocalAnchor(vec2(0.5, 0.5));
        _label->SetParent(this);
        _label->SetCollisionEnabled(false);
        _label->SetHorizontalAlignment(UI::TextHorizontalAlignment::CENTER);
    }
    
    void Button::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Button", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Button>("Transform");
        r = ScriptEngine::RegisterScriptFunction("Button@ CreateButton()", asFUNCTION(Button::CreateButton)); assert(r >= 0);

        //Button Functions.
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(Button, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void ButtonEventCallback(Button@ button)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(ButtonEventCallback@ cb)", asMETHOD(Button, SetOnClickCallback)); assert(r >= 0);

        //Label Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(Button, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(Button, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTextColor(Color color)", asMETHOD(Button, SetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetTextColor()", asMETHOD(Button, GetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(Button, SetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(Button, GetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(Button, SetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(Button, GetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(Button, SetTextFont)); assert(r >= 0);

        //Panel Functions.
        r = ScriptEngine::RegisterScriptClassFunction("string GetTexture()", asMETHOD(Button, GetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string Texture)", asMETHOD(Button, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(Button, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Button, SetColor)); assert(r >= 0);
    }

    void Button::SetOnClickCallback(asIScriptFunction* callback)
    {
        _events->onClickCallback = callback;
        _events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
    }

    void Button::SetText(const std::string& text)
    {
        _label->SetText(text);
    }
    const std::string Button::GetText() const
    {
        return _label->GetText();
    }

    void Button::SetTextColor(const Color& color)
    {
        _label->SetColor(color);
    }
    const Color& Button::GetTextColor() const
    {
        return _label->GetColor();
    }

    void Button::SetTextOutlineColor(const Color& outlineColor)
    {
        _label->SetOutlineColor(outlineColor);
    }
    const Color& Button::GetTextOutlineColor() const
    {
        return _label->GetOutlineColor();
    }

    void Button::SetTextOutlineWidth(f32 outlineWidth)
    {
        _label->SetOutlineWidth(outlineWidth);
    }
    const f32 Button::GetTextOutlineWidth() const
    {
        return _label->GetOutlineWidth();
    }

    void Button::SetTextFont(std::string fontPath, f32 fontSize)
    {
        _label->SetFont(fontPath, fontSize);
    }

    void Button::SetTexture(const std::string& texture)
    {
        _panel->SetTexture(texture);
    }

    const std::string& Button::GetTexture() const
    {
        return _panel->GetTexture();
    }

    void Button::SetColor(const Color& color)
    {
        _panel->SetColor(color);
    }

    const Color Button::GetColor() const
    {
        return _panel->GetColor();
    }

    Button* Button::CreateButton()
    {
        Button* button = new Button();

        return button;
    }
}