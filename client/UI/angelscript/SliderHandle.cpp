#include "SliderHandle.h"
#include "Slider.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Image.h"
#include "../ECS/Components/ImageEventStyles.h"
#include "../ECS/Components/Renderable.h"

namespace UIScripting
{
    SliderHandle::SliderHandle(Slider* owningSlider, const std::string& name, bool collisionEnabled) : _slider(owningSlider), EventElement(UI::ElementType::SLIDERHANDLE, name, collisionEnabled, UI::TransformEventsFlags::FLAG_DRAGGABLE | UI::TransformEventsFlags::FLAG_DRAGLOCK_Y)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->emplace<UIComponent::Image>(_entityId);
        registry->emplace<UIComponent::ImageEventStyles>(_entityId);
        registry->emplace<UIComponent::Renderable>(_entityId, UI::RenderType::Image);
    }
    
    void SliderHandle::OnDrag()
    {
        const UIComponent::Transform& transform = ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        _slider->OnClick(transform.anchorPosition + transform.position);
    }
}