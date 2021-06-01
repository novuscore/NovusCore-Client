#include "Panel.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Image.h"
#include "../ECS/Components/ImageEventStyles.h"
#include "../ECS/Components/Renderable.h"

#include <tracy/Tracy.hpp>

namespace UIScripting
{
    Panel::Panel(const std::string& name, bool collisionEnabled) : EventElement(UI::ElementType::PANEL, name, collisionEnabled)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::ImageEventStyles>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId, UI::RenderType::Image);
    }

    void Panel::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Panel", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
        r = RegisterEventBase<Panel>("Panel");

        r = ScriptEngine::RegisterScriptFunction("Panel@ CreatePanel(string name, bool collisionEnabled = true)", asFUNCTION(Panel::CreatePanel)); assert(r >= 0);

        // Renderable Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(ImageStylesheet styleSheet)", asMETHOD(Panel, SetStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFocusedStylesheet(ImageStylesheet styleSheet)", asMETHOD(Panel, SetFocusedStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetHoverStylesheet(ImageStylesheet styleSheet)", asMETHOD(Panel, SetHoverStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetPressedStylesheet(ImageStylesheet styleSheet)", asMETHOD(Panel, SetPressedStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetDisabledStylesheet(ImageStylesheet styleSheet)", asMETHOD(Panel, SetDisabledStylesheet)); assert(r >= 0);
    }

    void Panel::SetStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_NORMAL] = styleSheet;
    }
    void Panel::SetFocusedStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_FOCUSED] = styleSheet;
    }
    void Panel::SetHoverStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_HOVERED] = styleSheet;
    }
    void Panel::SetPressedStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_PRESSED] = styleSheet;
    }
    void Panel::SetDisabledStylesheet(const UI::ImageStylesheet& stylesheet)
    {
        UIComponent::ImageEventStyles* imageStyles = &ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        imageStyles->styleMap[UI::TransformEventState::STATE_DISABLED] = stylesheet;
    }

    Panel* Panel::CreatePanel(const std::string& name, bool collisionEnabled)
    {
        Panel* panel = new Panel(name, collisionEnabled);

        return panel;
    }
}