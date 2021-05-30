#include "ProgressBar.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/ProgressBar.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/TransformFill.h"

#include "Panel.h"

namespace UIScripting
{
    ProgressBar::ProgressBar(const std::string& name, bool collisionEnabled) : EventElement(UI::ElementType::UITYPE_SLIDER, name, collisionEnabled)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::ProgressBar>(_entityId);
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId, UI::RenderType::Image);

        _panel = Panel::CreatePanel(name + "-Panel", false);
        InternalAddChild(_panel);
        registry->emplace<UIComponent::TransformFill>(_panel->GetEntityId()).flags = UI::TransformFillFlags::FILL_PARENTSIZE;
    }

    void ProgressBar::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("ProgressBar", 0, asOBJ_REF | asOBJ_NOCOUNT);
        RegisterBase<ProgressBar>();
        r = ScriptEngine::RegisterScriptFunction("ProgressBar@ CreateProgressBar(string name, bool collisionEnabled)", asFUNCTION(ProgressBar::CreateProgressBar)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("float GetCurrentValue()", asMETHOD(ProgressBar, GetCurrentValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCurrentValue(float current)", asMETHOD(ProgressBar, SetCurrentValue)); assert(r >= 0);
        
        // Image Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(ImageStylesheet styleSheet)", asMETHOD(ProgressBar, SetStylesheet)); assert(r >= 0);
    }

    f32 ProgressBar::GetCurrentValue() const
    {
        const UIComponent::ProgressBar& progressBar = ServiceLocator::GetUIRegistry()->get<UIComponent::ProgressBar>(_entityId);
        return progressBar.currentValue;
    }
    void ProgressBar::SetCurrentValue(f32 current)
    {
        UIComponent::ProgressBar& progressBar = ServiceLocator::GetUIRegistry()->get<UIComponent::ProgressBar>(_entityId);
        progressBar.currentValue = current;
    }

    void ProgressBar::SetStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::Image& image = ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image.style = styleSheet;
    }

    ProgressBar* ProgressBar::CreateProgressBar(const std::string& name, bool collisionEnabled)
    {
        ProgressBar* progressBar = new ProgressBar(name, collisionEnabled);

        return progressBar;
    }
}