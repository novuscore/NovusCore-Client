#include "AddElementSystem.h"
#include <entt.hpp>
#include <tracy/Tracy.hpp>
#include "../../Components/UI/UIAddElementQueueSingleton.h"
#include "../../Components/UI/UITransform.h"
#include "../../Components/UI/UITransformEvents.h"
#include "../../Components/UI/UIRenderable.h"
#include "../../Components/UI/UIText.h"
#include "../../Components/UI/UIInputField.h"

void AddElementSystem::Update(entt::registry& registry)
{
    UIAddElementQueueSingleton& uiAddElementQueueSingleton = registry.ctx<UIAddElementQueueSingleton>();

    ZoneScopedNC("AddElementSystem::Update", tracy::Color::Blue)

    UIElementData element;
    while (uiAddElementQueueSingleton.elementPool.try_dequeue(element))
    {
        UITransform& transform = registry.assign<UITransform>(element.entityId);
        transform.type = element.type;
        transform.asObject = element.asObject;

        switch (element.type)
        {
        case UIElementType::UITYPE_TEXT:
            registry.assign<UIText>(element.entityId);
            break;
        case UIElementType::UITYPE_PANEL:
            registry.assign<UIRenderable>(element.entityId);
            break;
        case UIElementType::UITYPE_INPUTFIELD:
        {
            registry.assign<UIText>(element.entityId);
            UIInputField& inputField = registry.assign<UIInputField>(element.entityId);
            inputField.asObject = element.asObject;
            break;
        }
        default:
            break;
        }

        if (element.type != UIElementType::UITYPE_TEXT)
        {
            UITransformEvents& events = registry.assign<UITransformEvents>(element.entityId);
            events.asObject = element.asObject;
        }
    }
}