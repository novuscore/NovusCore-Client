#include "Label.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Renderable.h"

namespace UIScripting
{
    Label::Label() : BaseElement(UI::UIElementType::UITYPE_LABEL)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::Text>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Text;
    }

    void Label::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Label", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Label>("BaseElement");
        r = ScriptEngine::RegisterScriptFunction("Label@ CreateLabel()", asFUNCTION(Label::CreateLabel)); assert(r >= 0);

        //Text Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(Label, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(Label, SetFont)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(Label, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Label, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(Label, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(Label, SetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(Label, GetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(Label, SetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(Label, GetOutlineWidth)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void SetHorizontalAlignment(uint8 alignement)", asMETHOD(Label, SetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetVerticalAlignment(uint8 alignment)", asMETHOD(Label, SetOutlineWidth)); assert(r >= 0);
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

    void Label::SetFont(const std::string& fontPath, f32 fontSize)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->style.fontPath = fontPath;
        text->style.fontSize = fontSize;
    }

    const Color& Label::GetColor() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->style.outlineColor;
    }
    void Label::SetColor(const Color& color)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->style.color = color;
    }

    const Color& Label::GetOutlineColor() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->style.outlineColor;
    }
    void Label::SetOutlineColor(const Color& outlineColor)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->style.outlineColor = outlineColor;
    }

    const f32 Label::GetOutlineWidth() const
    {
        const UIComponent::Text* text = &ServiceLocator::GetUIRegistry()->get<UIComponent::Text>(_entityId);
        return text->style.outlineWidth;
    }
    void Label::SetOutlineWidth(f32 outlineWidth)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->style.outlineWidth = outlineWidth;
    }

    void Label::SetHorizontalAlignment(UI::TextHorizontalAlignment alignment)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->horizontalAlignment = alignment;
    }
    void Label::SetVerticalAlignment(UI::TextVerticalAlignment alignment)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Text* text = &registry->get<UIComponent::Text>(_entityId);
        text->verticalAlignment = alignment;
    }

    Label* Label::CreateLabel()
    {
        Label* label = new Label();

        return label;
    }
}