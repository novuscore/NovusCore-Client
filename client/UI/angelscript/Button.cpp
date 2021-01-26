#include "Button.h"
#include "Label.h"
#include <tracy/Tracy.hpp>
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Transformevents.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/Text.h"
#include "../ECS/Components/Renderable.h"

namespace UIScripting
{
    Button::Button() : BaseElement(UI::ElementType::UITYPE_BUTTON) 
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        registry->emplace<UIComponent::TransformEvents>(_entityId);
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;
        
        _label = Label::CreateLabel();
        InternalAddChild(_label);
        auto [labelTransform, labelText] = registry->get<UIComponent::Transform, UIComponent::Text>(_label->GetEntityId());
        labelTransform.SetFlag(UI::TransformFlags::FILL_PARENTSIZE);
        labelText.style.SetHorizontalAlignment(UI::TextHorizontalAlignment::CENTER);
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
        r = ScriptEngine::RegisterScriptClassFunction("void SetTextStylesheet(TextStylesheet stylesheet)", asMETHOD(Button, SetTextStylesheet)); assert(r >= 0);

        //Panel Functions.
        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(ImageStylesheet stylesheet)", asMETHOD(Button, SetStylesheet)); assert(r >= 0);
    }

    const bool Button::IsClickable() const
    {
        const UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        return events->HasFlag(UI::TransformEventsFlags::FLAG_CLICKABLE);
    }

    void Button::SetOnClickCallback(asIScriptFunction* callback)
    {
        UIComponent::TransformEvents* events = &ServiceLocator::GetUIRegistry()->get<UIComponent::TransformEvents>(_entityId);
        events->onClickCallback = callback;
        events->SetFlag(UI::TransformEventsFlags::FLAG_CLICKABLE);
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
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style = stylesheet;
    }

    Button* Button::CreateButton()
    {
        Button* button = new Button();

        return button;
    }
}