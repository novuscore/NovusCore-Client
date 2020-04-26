#include "AddElementSystem.h"
#include "../../Components/UI/UIAddElementQueueSingleton.h"
#include "../../Components/UI/UITransform.h"
#include "../../Components/UI/UITransformEvents.h"
#include "../../Components/UI/UIRenderable.h"
#include "../../Components/UI/UIText.h"
#include <tracy/Tracy.hpp>

void AddElementSystem::Update(entt::registry& registry)
{
    UIAddElementQueueSingleton& uiAddElementQueueSingleton = registry.ctx<UIAddElementQueueSingleton>();

    ZoneScopedNC("AddElementSystem::Update", tracy::Color::Blue)

        UIElementData element;
    while (uiAddElementQueueSingleton.elementPool.try_dequeue(element))
    {
        registry.assign<UITransform>(element.entityId);

        if (element.type == UIElementData::UIElementType::UITYPE_TEXT)
        {
            registry.assign<UIText>(element.entityId);
        }
        else
        {
            UITransformEvents& events = registry.assign<UITransformEvents>(element.entityId);
            events.asObject = element.asObject;

            registry.assign<UIRenderable>(element.entityId);
        }
    }
}