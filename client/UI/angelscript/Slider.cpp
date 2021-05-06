#include "Slider.h"
#include "SliderHandle.h"
#include <GLFW/glfw3.h>
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/ImageEventStyles.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Slider.h"
#include "../Utils/TransformUtils.h"
#include "../Utils/SliderUtils.h"
#include "../Utils/EventUtils.h"

namespace UIScripting
{
    Slider::Slider() : EventElement(UI::ElementType::UITYPE_SLIDER, true, UI::TransformEventsFlags::FLAG_CLICKABLE)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::Slider>(_entityId);
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::ImageEventStyles>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId, UI::RenderType::Image);

        _handle = new SliderHandle(this);
        InternalAddChild(_handle);
        auto handleTransform = &registry->get<UIComponent::Transform>(_handle->GetEntityId());
        handleTransform->localAnchor = vec2(0.5f, 0.5f);
    }

    void Slider::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Slider", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = RegisterEventBase<Slider>("Slider");
        r = ScriptEngine::RegisterScriptFunction("Slider@ CreateSlider()", asFUNCTION(Slider::CreateSlider)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("float GetCurrentValue()", asMETHOD(Slider, GetCurrentValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetCurrentValue(float current)", asMETHOD(Slider, SetCurrentValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetMinValue()", asMETHOD(Slider, GetMinValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetMinValue(float min)", asMETHOD(Slider, SetMinValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetMaxValue()", asMETHOD(Slider, GetMaxValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetMaxValue(float max)", asMETHOD(Slider, SetMaxValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetPercentValue()", asMETHOD(Slider, SetMaxValue)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetPercentValue(float percent)", asMETHOD(Slider, SetMaxValue)); assert(r >= 0);

        // Image Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetStylesheet(ImageStylesheet styleSheet)", asMETHOD(Slider, SetStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFocusedStylesheet(ImageStylesheet styleSheet)", asMETHOD(Slider, SetFocusedStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetHoverStylesheet(ImageStylesheet styleSheet)", asMETHOD(Slider, SetHoverStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetPressedStylesheet(ImageStylesheet styleSheet)", asMETHOD(Slider, SetPressedStylesheet)); assert(r >= 0);

        // Handle functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetHandleStylesheet(ImageStylesheet styleSheet)", asMETHOD(Slider, SetHandleStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetHandleFocusedStylesheet(ImageStylesheet styleSheet)", asMETHOD(Slider, SetHandleFocusedStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetHandleHoverStylesheet(ImageStylesheet styleSheet)", asMETHOD(Slider, SetHandleHoverStylesheet)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetHandlePressedStylesheet(ImageStylesheet styleSheet)", asMETHOD(Slider, SetHandlePressedStylesheet)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("float GetStepSize()", asMETHOD(Slider, GetStepSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetStepSize(float stepSize)", asMETHOD(Slider, SetStepSize)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void OnValueChange(SliderEventCallback@ cb)", asMETHOD(Slider, SetOnValueChangedCallback)); assert(r >= 0);
    }

    f32 Slider::GetMinValue() const
    {
        const UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider.minimumValue;
    }
    void Slider::SetMinValue(f32 min)
    {
        UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider.minimumValue = min;
    
        UpdateHandlePosition();
    }

    f32 Slider::GetMaxValue() const
    {
        const UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider.maximumValue;
    }
    void Slider::SetMaxValue(f32 max)
    {
        UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider.maximumValue = max;

        UpdateHandlePosition();
    }

    f32 Slider::GetCurrentValue() const
    {
        const UIComponent::Slider slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider.currentValue;
    }
    void Slider::SetCurrentValue(f32 current)
    {
        UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider.currentValue = current;

        UpdateHandlePosition();
        UIUtils::ExecuteEvent(this, slider.onValueChanged);
    }

    f32 Slider::GetPercentValue() const
    {
        const UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return UIUtils::Slider::GetPercent(slider);
    }
    void Slider::SetPercentValue(f32 value)
    {
        UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider.currentValue = UIUtils::Slider::GetValueFromPercent(slider, value);
    }

    f32 Slider::GetStepSize() const
    {
        const UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider.stepSize;
    }
    void Slider::SetStepSize(f32 stepSize)
    {
        UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider.stepSize = stepSize;
    }

    void Slider::SetOnValueChangedCallback(asIScriptFunction* callback)
    {
        UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider.onValueChanged = callback;
    }

    void Slider::SetStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_NORMAL] = styleSheet;
    }
    void Slider::SetFocusedStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_FOCUSED] = styleSheet;
    }
    void Slider::SetHoverStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_HOVERED] = styleSheet;
    }
    void Slider::SetPressedStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_entityId);
        image.styleMap[UI::TransformEventState::STATE_PRESSED] = styleSheet;
    }

    void Slider::SetHandleStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_handle->GetEntityId());
        image.styleMap[UI::TransformEventState::STATE_NORMAL] = styleSheet;
    }
    void Slider::SetHandleFocusedStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_handle->GetEntityId());
        image.styleMap[UI::TransformEventState::STATE_FOCUSED] = styleSheet;
    }
    void Slider::SetHandleHoverStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_handle->GetEntityId());
        image.styleMap[UI::TransformEventState::STATE_PRESSED] = styleSheet;
    }
    void Slider::SetHandlePressedStylesheet(const UI::ImageStylesheet& styleSheet)
    {
        UIComponent::ImageEventStyles& image = ServiceLocator::GetUIRegistry()->get<UIComponent::ImageEventStyles>(_handle->GetEntityId());
        image.styleMap[UI::TransformEventState::STATE_PRESSED] = styleSheet;
    }

    void Slider::OnClick(vec2 mousePosition)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto [transform, slider] = registry->get<UIComponent::Transform, UIComponent::Slider>(_entityId);
        UIComponent::Transform& handleTransform = registry->get<UIComponent::Transform>(_handle->GetEntityId());

        const vec2 minBounds = UIUtils::Transform::GetMinBounds(transform);
        const vec2 maxBounds = UIUtils::Transform::GetMaxBounds(transform);
        
        f32 percent = Math::Clamp((mousePosition.x - minBounds.x) / (maxBounds.x - minBounds.x), 0.f, 1.f);
        f32 newValue = UIUtils::Slider::GetValueFromPercent(slider, percent);

        if (slider.stepSize != 0.f)
        {
            newValue = static_cast<i32>(newValue / slider.stepSize) * slider.stepSize;
            percent = (newValue - slider.minimumValue) / (slider.maximumValue - slider.minimumValue);
        }

        slider.currentValue = newValue;

        // Update slider position.
        handleTransform.position = vec2(0.0f, 0.0f);
        handleTransform.anchor = vec2(percent, 0.5f);
        handleTransform.anchorPosition = UIUtils::Transform::GetAnchorPositionInElement(transform, handleTransform.anchor);
        _handle->MarkBoundsDirty();
        _handle->MarkDirty();
        
        UIUtils::ExecuteEvent(this, slider.onValueChanged);
    }

    bool Slider::OnKeyInput(i32 key)
    {
        if (key == GLFW_KEY_LEFT)
        {
            entt::registry* registry = ServiceLocator::GetUIRegistry();
            UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);

            slider.currentValue = Math::Max(slider.currentValue - slider.stepSize, slider.minimumValue);

            return true;
        }
        else if (key == GLFW_KEY_RIGHT)
        {
            entt::registry* registry = ServiceLocator::GetUIRegistry();
            UIComponent::Slider& slider = ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);

            slider.currentValue = Math::Min(slider.currentValue + slider.stepSize, slider.maximumValue);

            return true;
        }

        return false;
    }

    void Slider::SetHandleSize(const vec2& size)
    {
        _handle->SetSize(size);
    }
    void Slider::UpdateHandlePosition()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto [slider, transform] = registry->get<UIComponent::Slider, UIComponent::Transform>(_entityId);
        UIComponent::Transform& handleTransform = registry->get<UIComponent::Transform>(_handle->GetEntityId());

        handleTransform.position = vec2(0.0f, 0.0f);
        handleTransform.anchor = vec2(UIUtils::Slider::GetPercent(slider), 0.5f);
        handleTransform.anchorPosition = UIUtils::Transform::GetAnchorPositionInElement(transform, handleTransform.anchor);
        _handle->MarkBoundsDirty();
        _handle->MarkDirty();
    }

    Slider* Slider::CreateSlider()
    {
        Slider* slider = new Slider();
        return slider;
    }
}