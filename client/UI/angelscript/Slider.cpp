#include "Slider.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Collidable.h"
#include "../ECS/Components/Slider.h"

#include "SliderHandle.h"

namespace UIScripting
{
    Slider::Slider() : BaseElement(UI::UIElementType::UITYPE_SLIDER)
    { 
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        UISingleton::UILockSingleton& uiLockSingleton = registry->ctx<UISingleton::UILockSingleton>();
        uiLockSingleton.mutex.lock();
        {
            UIComponent::Transform* transform = &registry->emplace<UIComponent::Transform>(_entityId);
            transform->sortData.entId = _entityId;
            transform->sortData.type = _elementType;
            transform->asObject = this;

            UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
            events->asObject = this;

            registry->emplace<UIComponent::Visible>(_entityId);
            registry->emplace<UIComponent::Visibility>(_entityId);
            registry->emplace<UIComponent::Image>(_entityId);
            registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;

            transform->collision = true;
            registry->emplace<UIComponent::Collidable>(_entityId);

            registry->emplace<UIComponent::Slider>(_entityId);
        }
        uiLockSingleton.mutex.unlock();

        sliderHandle = SliderHandle::CreateSliderHandle(this);
    }

    void Slider::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Slider", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Slider>("BaseElement");
        r = ScriptEngine::RegisterScriptFunction("Slider@ CreateSlider()", asFUNCTION(Slider::CreateSlider)); assert(r >= 0);
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
        slider->maximumValue = current;

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
        // TODO: insert return statement here
    }
    void Slider::SetHandleTexture(const std::string& texture)
    {
    }

    const Color Slider::GetHandleColor() const
    {
        return Color();
    }
    void Slider::SetHandleColor(const Color& color)
    {
    }

    Slider* Slider::CreateSlider()
    {
        Slider* slider = new Slider();
        
        return slider;
    }
}