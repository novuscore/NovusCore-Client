#include "Slider.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Collidable.h"
#include "../ECS/Components/Slider.h"

#include "SliderHandle.h"

namespace UIScripting
{
    Slider::Slider() : BaseElement(UI::UIElementType::UITYPE_SLIDER)
    {
        ZoneScoped;
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
        events->asObject = this;

        registry->emplace<UIComponent::Slider>(_entityId);
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;

        _handle = SliderHandle::CreateSliderHandle(this);
        UIComponent::Transform* handleTransform = &registry->get<UIComponent::Transform>(_handle->GetEntityId());
        handleTransform->parent = _entityId;
        registry->get<UIComponent::SortKey>(_handle->GetEntityId()).data.depth++;
        registry->get<UIComponent::Transform>(_entityId).children.push_back({ _handle->GetEntityId(), _handle->GetType() });
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
    }

    float Slider::GetMinValue()
    {
        const UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider->minimumValue;
    }
    void Slider::SetMinValue(float min)
    {
        UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider->minimumValue = min;
        slider->currentValue = Math::Max(slider->currentValue, slider->minimumValue);
        //TODO Update handle position.
    }

    float Slider::GetMaxValue()
    {
        const UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider->maximumValue;
    }
    void Slider::SetMaxValue(float max)
    {
        UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider->maximumValue = max;
        slider->currentValue = Math::Min(slider->currentValue, slider->maximumValue);
        //TODO Update handle position.
    }

    float Slider::GetCurrentValue()
    {
        const UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        return slider->currentValue;
    }
    void Slider::SetCurrentValue(float current)
    {
        UIComponent::Slider* slider = &ServiceLocator::GetUIRegistry()->get<UIComponent::Slider>(_entityId);
        slider->currentValue = Math::Clamp(current, slider->minimumValue, slider->maximumValue);

        //TODO Update handle position.
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

    Slider* Slider::CreateSlider()
    {
        Slider* slider = new Slider();

        return slider;
    }
}