#include "SliderHandle.h"
#include "Slider.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/Renderable.h"

namespace UIScripting
{

    SliderHandle::SliderHandle(Slider* owningSlider) : _slider(owningSlider), BaseElement(UI::ElementType::UITYPE_SLIDERHANDLE)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
        events->SetFlag(static_cast<UI::TransformEventsFlags>(UI::TransformEventsFlags::FLAG_DRAGGABLE | UI::TransformEventsFlags::FLAG_DRAGLOCK_Y));

        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;
    }    
    
    void SliderHandle::OnDragged()
    {
        const UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        _slider->OnClicked(transform->anchorPosition + transform->position);
    }

    SliderHandle* SliderHandle::CreateSliderHandle(Slider* owningSlider)
    {
        SliderHandle* sliderHandle = new SliderHandle(owningSlider);

        return sliderHandle;
    }
}