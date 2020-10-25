#include "Button.h"
#include "Label.h"
#include <tracy/Tracy.hpp>
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Transformevents.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/SortKey.h"

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
        _label->SetHorizontalAlignment(UI::TextHorizontalAlignment::CENTER);
        auto labelTransform = &registry->get<UIComponent::Transform>(_label->GetEntityId());
        labelTransform->parent = _entityId;
        labelTransform->SetFlag(UI::TransformFlags::FILL_PARENTSIZE);
        registry->get<UIComponent::SortKey>(_label->GetEntityId()).data.depth++;
        registry->get<UIComponent::Transform>(_entityId).children.push_back({ _label->GetEntityId(), _label->GetType() });
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
        r = ScriptEngine::RegisterScriptClassFunction("string GetBorder()", asMETHOD(Button, GetBorder)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorder(string Texture)", asMETHOD(Button, SetBorder)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderSize(uint topSize, uint rightSize, uint bottomSize, uint leftSize)", asMETHOD(Button, SetBorderSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderInset(uint topBorderInset, uint rightBorderInset, uint bottomBorderInset, uint leftBorderInset)", asMETHOD(Button, SetBorderInset)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetSlicing(uint topOffset, uint rightOffset, uint bottomOffset, uint leftOffset)", asMETHOD(Button, SetSlicing)); assert(r >= 0);
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
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
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

    const std::string& Button::GetBorder() const
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.border;
    }
    void Button::SetBorder(const std::string& texture)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.border = texture;
    }

    void Button::SetBorderSize(const u32 topSize, const u32 rightSize, const u32 bottomSize, const u32 leftSize)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.borderSize.top = topSize;
        image->style.borderSize.right = rightSize;
        image->style.borderSize.bottom = bottomSize;
        image->style.borderSize.left = leftSize;
    }
    void Button::SetBorderInset(const u32 topBorderInset, const u32 rightBorderInset, const u32 bottomBorderInset, const u32 leftBorderInset)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.borderInset.top = topBorderInset;
        image->style.borderInset.right = rightBorderInset;
        image->style.borderInset.bottom = bottomBorderInset;
        image->style.borderInset.left = leftBorderInset;
    }

    void Button::SetSlicing(const u32 topOffset, const u32 rightOffset, const u32 bottomOffset, const u32 leftOffset)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.slicingOffset.top = topOffset;
        image->style.slicingOffset.right = rightOffset;
        image->style.slicingOffset.bottom = bottomOffset;
        image->style.slicingOffset.left = leftOffset;
    }

    Button* Button::CreateButton()
    {
        Button* button = new Button();

        return button;
    }
}