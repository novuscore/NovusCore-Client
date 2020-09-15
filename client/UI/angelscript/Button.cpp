#include "Button.h"
#include "Label.h"
#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Collidable.h"
#include "../ECS/Components/Renderable.h"

namespace UIScripting
{
    Button::Button() : BaseElement(UI::UIElementType::UITYPE_BUTTON) 
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
        events->asObject = this;

        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;
        
        // Not locking is fine here because no one else can reasonably modify this.
        _label = Label::CreateLabel();
        _label->SetCollisionEnabled(false);
        _label->SetHorizontalAlignment(UI::TextHorizontalAlignment::CENTER);
        _label->SetFillParentSize(true);
        _label->SetParent(this);
    }
    
    void Button::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Button", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Button>("BaseElement");
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
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(Button, SetFont)); assert(r >= 0);

        //Panel Functions.
        r = ScriptEngine::RegisterScriptClassFunction("string GetTexture()", asMETHOD(Button, GetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string Texture)", asMETHOD(Button, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(Button, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Button, SetColor)); assert(r >= 0);
    }

    const bool Button::IsClickable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->IsClickable();
    }

    void Button::SetOnClickCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onClickCallback = callback;
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
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

    void Button::SetFont(std::string fontPath, f32 fontSize)
    {
        _label->SetFont(fontPath, fontSize);
    }

    const std::string& Button::GetTexture() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.texture;
    }
    void Button::SetTexture(const std::string& texture)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Image* image = &registry->get<UIComponent::Image>(_entityId);
        image->style.texture = texture;
    }

    const Color Button::GetColor() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.color;
    }
    void Button::SetColor(const Color& color)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Image* image = &registry->get<UIComponent::Image>(_entityId);
        image->style.color = color;
    }

    Button* Button::CreateButton()
    {
        Button* button = new Button();

        return button;
    }
}