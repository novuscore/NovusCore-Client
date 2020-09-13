#include "SliderHandle.h"
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Renderable.h"
#include "../ECS/Components/Collidable.h"

namespace UIScripting
{
    SliderHandle::SliderHandle(Slider* owningSlider) : _slider(owningSlider), BaseElement(UI::UIElementType::UITYPE_SLIDERHANDLE)
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
        }
        uiLockSingleton.mutex.unlock();
    }

    SliderHandle* SliderHandle::CreateSliderHandle(Slider* owningSlider)
    {
        SliderHandle* sliderHandle = new SliderHandle(owningSlider);

        return sliderHandle;
    }
}