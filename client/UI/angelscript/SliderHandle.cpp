#include "SliderHandle.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/Renderable.h"

namespace UIScripting
{

    SliderHandle::SliderHandle(Slider* owningSlider) : _slider(owningSlider), BaseElement(UI::ElementType::UITYPE_SLIDERHANDLE)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::TransformEvents* events = &registry->emplace<UIComponent::TransformEvents>(_entityId);
        events->SetFlag(UI::UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
        events->dragLockY = true;

        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId).renderType = UI::RenderType::Image;
    }    
    
    void SliderHandle::OnDragged()
    {
        // TODO Limit position to inside slider.
        // TODO Notify slider.
    }

    SliderHandle* SliderHandle::CreateSliderHandle(Slider* owningSlider)
    {
        SliderHandle* sliderHandle = new SliderHandle(owningSlider);

        return sliderHandle;
    }
}