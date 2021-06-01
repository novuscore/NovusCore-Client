#include "Button.h"
#include "Label.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/TransformFill.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/ImageEventStyles.h"
#include "../ECS/Components/Renderable.h"

namespace UIScripting
{
    Button::Button(const std::string& name, bool collisionEnabled) : EventElement(UI::ElementType::BUTTON, name, collisionEnabled, UI::TransformEventsFlags::FLAG_CLICKABLE) 
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::ImageEventStyles>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId, UI::RenderType::Image);
        
        _label = Label::CreateLabel(name + "-Label");
        InternalAddChild(_label);
        registry->emplace<UIComponent::TransformFill>(_label->GetEntityId()).flags = UI::TransformFillFlags::FILL_PARENTSIZE;
    }
    
    void Button::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Button", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
        r = RegisterEventBase<Button>("Button");

        r = ScriptEngine::RegisterScriptFunction("Button@ CreateButton(string name, bool collisionEnabled = true)", asFUNCTION(Button::CreateButton)); assert(r >= 0);

        //Label Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(Button, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTextStylesheet(TextStylesheet stylesheet)", asMETHOD(Button, SetTextStylesheet)); assert(r >= 0);

        //Panel Functions.
        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(ImageStylesheet stylesheet)", asMETHOD(Button, SetStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFocusedStylesheet(ImageStylesheet stylesheet)", asMETHOD(Button, SetFocusedStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetHoveredStylesheet(ImageStylesheet stylesheet)", asMETHOD(Button, SetHoveredStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetPressedStylesheet(ImageStylesheet stylesheet)", asMETHOD(Button, SetPressedStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetDisabledStylesheet(ImageStylesheet stylesheet)", asMETHOD(Button, SetDisabledStylesheet)); assert(r >= 0);
    }

    const std::string Button::GetText() const
    {
        return _label->GetText();
    }
    void Button::SetText(const std::string& text)
    {
        _label->SetText(text);
    }

    void Button::SetTextStylesheet(const UI::TextStylesheet& textStylesheet)
    {
        _label->SetStylesheet(textStylesheet);
    }

    void Button::SetStylesheet(const UI::ImageStylesheet& stylesheet)
    {
        UIComponent::ImageEventStyles* imageStyles = &ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        imageStyles->styleMap[UI::TransformEventState::STATE_NORMAL] = stylesheet;
    }
    void Button::SetFocusedStylesheet(const UI::ImageStylesheet& stylesheet)
    {
        UIComponent::ImageEventStyles* imageStyles = &ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        imageStyles->styleMap[UI::TransformEventState::STATE_FOCUSED] = stylesheet;
    }
    void Button::SetHoveredStylesheet(const UI::ImageStylesheet& stylesheet)
    {
        UIComponent::ImageEventStyles* imageStyles = &ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        imageStyles->styleMap[UI::TransformEventState::STATE_HOVERED] = stylesheet;
    }
    void Button::SetPressedStylesheet(const UI::ImageStylesheet& stylesheet)
    {
        UIComponent::ImageEventStyles* imageStyles = &ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        imageStyles->styleMap[UI::TransformEventState::STATE_PRESSED] = stylesheet;
    }
    void Button::SetDisabledStylesheet(const UI::ImageStylesheet& stylesheet)
    {
        UIComponent::ImageEventStyles* imageStyles = &ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        imageStyles->styleMap[UI::TransformEventState::STATE_DISABLED] = stylesheet;
    }

    Button* Button::CreateButton(const std::string& name, bool collisionEnabled)
    {
        Button* button = new Button(name, collisionEnabled);

        return button;
    }
}