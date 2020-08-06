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

        UISingleton::UILockSingleton& uiLockSingleton = registry->ctx<UISingleton::UILockSingleton>();
        uiLockSingleton.mutex.lock();
        {
            _transform = &registry->emplace<UIComponent::Transform>(_entityId);
            _transform->sortData.entId = _entityId;
            _transform->sortData.type = _elementType;
            _transform->asObject = this;

            registry->emplace<UIComponent::Visible>(_entityId);
            _visibility = &registry->emplace<UIComponent::Visibility>(_entityId);
            _text = &registry->emplace<UIComponent::Text>(_entityId);
            registry->emplace<UIComponent::Renderable>(_entityId);
        }
        uiLockSingleton.mutex.unlock();
    }
    
    void Label::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Label", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Label>("Transform");
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
    }

    void Label::SetText(const std::string& text)
    {
        _text->text = text;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void Label::SetFont(const std::string& fontPath, f32 fontSize)
    {
        _text->fontPath = fontPath;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void Label::SetColor(const Color& color)
    {
        _text->color = color;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void Label::SetOutlineColor(const Color& outlineColor)
    {
        _text->outlineColor = outlineColor;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void Label::SetOutlineWidth(f32 outlineWidth)
    {
        _text->outlineWidth = outlineWidth;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void Label::SetHorizontalAlignment(UI::TextHorizontalAlignment alignment)
    {
        _text->horizontalAlignment = alignment;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    void Label::SetVerticalAlignment(UI::TextVerticalAlignment alignment)
    {
        _text->verticalAlignment = alignment;

        MarkDirty(ServiceLocator::GetUIRegistry(), _entityId);
    }

    Label* Label::CreateLabel()
    {
        Label* label = new Label();

        return label;
    }
}