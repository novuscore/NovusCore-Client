#include "Slider.h"
#include "SliderHandle.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Relation.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Slider.h"
#include "../Utils/TransformUtils.h"
#include "../Utils/SliderUtils.h"
#include "../Utils/EventUtils.h"


namespace UIScripting
{
    Slider::Slider() : BaseElement(UI::ElementType::UITYPE_SLIDER)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
        events->SetFlag(UI::TransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);

        registry->emplace<UIComponent::Slider>(_entityId);
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;

        _handle = SliderHandle::CreateSliderHandle(this);
        auto [handleTransform, handleRelation, handleSortKey] = registry->get<UIComponent::Transform, UIComponent::Relation, UIComponent::SortKey>(_handle->GetEntityId());
        handleTransform.SetFlag(UI::TransformFlags::FILL_PARENTSIZE);
        handleTransform.localAnchor = vec2(0.5f, 0.5f);
        handleRelation.parent = _entityId;
        handleSortKey.data.depth++;

        registry->get<UIComponent::Relation>(_entityId).children.push_back({ _handle->GetEntityId(), _handle->GetType() });
    }

    void Slider::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Slider", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Slider>("BaseElement");
        r = ScriptEngine::RegisterScriptFunction("Slider@ CreateSlider()", asFUNCTION(Slider::CreateSlider)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(Slider, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Slider, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetHandleTexture(string texture)", asMETHOD(Slider, SetHandleTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetHandleColor(Color color)", asMETHOD(Slider, SetHandleColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetHandleSize(vec2 size)", asMETHOD(Slider, SetHandleSize)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("float GetCurrentValue()", asMETHOD(Slider, GetCurrentValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCurrentValue(float current)", asMETHOD(Slider, SetCurrentValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetMinValue()", asMETHOD(Slider, GetMinValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetMinValue(float min)", asMETHOD(Slider, SetMinValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetMaxValue()", asMETHOD(Slider, GetMaxValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetMaxValue(float max)", asMETHOD(Slider, SetMaxValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetPercentValue()", asMETHOD(Slider, SetMaxValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetPercentValue(float percent)", asMETHOD(Slider, SetMaxValue)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("float GetStepSize()", asMETHOD(Slider, GetStepSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetStepSize(float stepSize)", asMETHOD(Slider, SetStepSize)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptFunctionDef("void SliderEventCallback(Slider@ slider)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnValueChange(SliderEventCallback@ cb)", asMETHOD(Slider, SetOnValueChangedCallback)); assert(r >= 0);
    }

    f32 Slider::GetMinValue() const
    {
        const UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider->minimumValue;
    }
    void Slider::SetMinValue(f32 min)
    {
        UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider->minimumValue = min;
        slider->currentValue = Math::Max(slider->currentValue, slider->minimumValue);
    
        UpdateHandlePosition();
    }

    f32 Slider::GetMaxValue() const
    {
        const UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider->maximumValue;
    }
    void Slider::SetMaxValue(f32 max)
    {
        UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider->maximumValue = max;
        slider->currentValue = Math::Min(slider->currentValue, slider->maximumValue);

        UpdateHandlePosition();
    }

    f32 Slider::GetCurrentValue() const
    {
        const UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider->currentValue;
    }
    void Slider::SetCurrentValue(f32 current)
    {
        UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider->currentValue = Math::Clamp(current, slider->minimumValue, slider->maximumValue);

        UpdateHandlePosition();
    }

    f32 Slider::GetPercentValue() const
    {
        const UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return UIUtils::Slider::GetPercent(slider);
    }
    void Slider::SetPercentValue(f32 value)
    {
        UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider->currentValue = UIUtils::Slider::GetValueFromPercent(slider, value);
    }

    f32 Slider::GetStepSize() const
    {
        const UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider->stepSize;
    }
    void Slider::SetStepSize(f32 stepSize)
    {
        UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider->stepSize = stepSize;
    }

    const std::string& Slider::GetTexture() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.texture;
    }
    void Slider::SetTexture(const std::string& texture)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.texture = texture;
    }

    const Color Slider::GetColor() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        return image->style.color;
    }
    void Slider::SetColor(const Color& color)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_entityId);
        image->style.color = color;
    }

    const std::string& Slider::GetHandleTexture() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_handle->GetEntityId());
        return image->style.texture;
    }
    void Slider::SetHandleTexture(const std::string& texture)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_handle->GetEntityId());
        image->style.texture = texture;
    }

    const Color Slider::GetHandleColor() const
    {
        const UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_handle->GetEntityId());
        return image->style.color;
    }
    void Slider::SetHandleColor(const Color& color)
    {
        UIComponent::Image* image = &ServiceLocator::GetUIRegistry()->get<UIComponent::Image>(_handle->GetEntityId());
        image->style.color = color;
    }

    void Slider::SetHandleSize(const vec2& size)
    {
        _handle->SetSize(size);
    }

    void Slider::OnClicked(hvec2 mousePosition)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        const UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(_entityId);
        UIComponent::Slider* slider = &registry->get<UIComponent::Slider>(_entityId);
        UIComponent::Transform* handleTransform = &registry->get<UIComponent::Transform>(_handle->GetEntityId());

        vec2 minBounds = UIUtils::Transform::GetMinBounds(transform);
        vec2 maxBounds = UIUtils::Transform::GetMaxBounds(transform);
        
        f32 percent = Math::Clamp((mousePosition.x - minBounds.x) / (maxBounds.x - minBounds.x), 0.f, 1.f);
        f32 newValue = UIUtils::Slider::GetValueFromPercent(slider, percent);

        if (slider->stepSize != 0.f)
        {
            newValue = (int)(newValue / slider->stepSize) * slider->stepSize;
            percent = (newValue - slider->minimumValue) / (slider->maximumValue - slider->minimumValue);
        }

        slider->currentValue = newValue;

        // Update slider position.
        handleTransform->position = vec2(0.0f, 0.0f);
        _handle->SetAnchor(vec2(percent, 0.5f));
        _handle->MarkBoundsDirty();
        _handle->MarkDirty();
        
        UIUtils::ExecuteEvent(this, slider->onValueChanged);
    }

    void Slider::SetOnValueChangedCallback(asIScriptFunction* callback)
    {
        UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider->onValueChanged = callback;
    }

    void Slider::UpdateHandlePosition()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        const UIComponent::Slider* slider = &registry->get<UIComponent::Slider>(_entityId);
        UIComponent::Transform* handleTransform = &registry->get<UIComponent::Transform>(_handle->GetEntityId());

        handleTransform->position = vec2(0.0f, 0.0f);
        _handle->SetAnchor(vec2(UIUtils::Slider::GetPercent(slider), 0.5f));
        _handle->MarkBoundsDirty();
        _handle->MarkDirty();
    }

    Slider* Slider::CreateSlider()
    {
        Slider* slider = new Slider();

        return slider;
    }
}