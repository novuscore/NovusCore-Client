#include "Label.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Text.h"
#include "../ECS/Components/Renderable.h"

#include <tracy/Tracy.hpp>

namespace UIScripting
{
    Label::Label(const std::string& name) : BaseElement(UI::ElementType::UITYPE_LABEL, name, false)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::Text>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId, UI::RenderType::Text);
    }

    void Label::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Label", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Label>("BaseElement");
        r = ScriptEngine::RegisterScriptFunction("Label@ CreateLabel(string name)", asFUNCTION(Label::CreateLabel)); assert(r >= 0);

        //Text Functions
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(Label, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(Label, SetText)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(TextStylesheet stylesheet)", asMETHOD(Label, SetStylesheet)); assert(r >= 0);
    }

    const std::string Label::GetText() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->text;
    }
    void Label::SetText(const std::string& newText)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->text = newText;
    }

    void Label::SetStylesheet(const UI::TextStylesheet& textStylesheet)
    {
        UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        text->style = textStylesheet;
    }

    Label* Label::CreateLabel(const std::string& name)
    {
        Label* label = new Label(name);

        return label;
    }
}