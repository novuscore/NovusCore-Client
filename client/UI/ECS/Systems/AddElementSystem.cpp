#include "AddElementSystem.h"
#include <entity/registry.hpp>
#include <entity/entity.hpp>
#include <tracy/Tracy.hpp>

#include "../Components/Singletons/UIAddElementQueueSingleton.h"
#include "../Components/Transform.h"
#include "../Components/TransformEvents.h"
#include "../Components/Renderable.h"
#include "../Components/Image.h"
#include "../Components/Text.h"
#include "../Components/Collidable.h"
#include "../Components/Visible.h"
#include "../Components/Visibility.h"
#include "../Components/InputField.h"
#include "../Components/Checkbox.h"

namespace UISystem
{
    void AddElementSystem::Update(entt::registry& registry)
    {
        UISingleton::UIAddElementQueueSingleton& uiAddElementQueueSingleton = registry.ctx<UISingleton::UIAddElementQueueSingleton>();

        UI::UIElementCreationData element;
        while (uiAddElementQueueSingleton.elementPool.try_dequeue(element))
        {
            UIComponent::Transform& transform = registry.emplace<UIComponent::Transform>(element.entityId);
            transform.sortData.entId = element.entityId;
            transform.sortData.type = element.type;
            transform.asObject = element.asObject;

            registry.emplace<UIComponent::Visible>(element.entityId);
            registry.emplace<UIComponent::Visibility>(element.entityId);

            switch (element.type)
            {
            case UI::UIElementType::UITYPE_LABEL:
                registry.emplace<UIComponent::Text>(element.entityId);
                break;
            case UI::UIElementType::UITYPE_PANEL:
                registry.emplace<UIComponent::Image>(element.entityId);
                break;
            case UI::UIElementType::UITYPE_INPUTFIELD:
            {
                registry.emplace<UIComponent::Text>(element.entityId);
                UIComponent::InputField& inputField = registry.emplace<UIComponent::InputField>(element.entityId);
                inputField.asObject = element.asObject;
                break;
            }
            case UI::UIElementType::UITYPE_CHECKBOX:
            {
                registry.emplace<UIComponent::Image>(element.entityId);
                UIComponent::Checkbox& checkBox = registry.emplace<UIComponent::Checkbox>(element.entityId);
                checkBox.asObject = element.asObject;
                break;
            }
            default:
                break;
            }

            if (registry.any<UIComponent::Image, UIComponent::Text>(element.entityId))
                registry.emplace<UIComponent::Renderable>(element.entityId);

            if (element.type != UI::UIElementType::UITYPE_LABEL)
            {
                registry.emplace<UIComponent::Collidable>(element.entityId);
                UIComponent::TransformEvents& events = registry.emplace<UIComponent::TransformEvents>(element.entityId);
                events.asObject = element.asObject;
            }
        }
    }
}