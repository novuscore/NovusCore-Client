#include "AddElementSystem.h"
#include <entity/registry.hpp>
#include <entity/entity.hpp>
#include <tracy/Tracy.hpp>

#include "../../Components/UI/Singletons/UIAddElementQueueSingleton.h"
#include "../../Components/UI/UITransform.h"
#include "../../Components/UI/UITransformEvents.h"

#include "../../Components/UI/UIRenderable.h"
#include "../../Components/UI/UIImage.h"
#include "../../Components/UI/UIText.h"

#include "../../Components/UI/UICollision.h"

#include "../../Components/UI/UIVisible.h"
#include "../../Components/UI/UIVisibility.h"

#include "../../Components/UI/UIInputField.h"

void AddElementSystem::Update(entt::registry& registry)
{
    ZoneScopedNC("AddElementSystem::Update", tracy::Color::Blue);

    UI::UIAddElementQueueSingleton& uiAddElementQueueSingleton = registry.ctx<UI::UIAddElementQueueSingleton>();

    UI::UIElementCreationData element;
    while (uiAddElementQueueSingleton.elementPool.try_dequeue(element))
    {
        UITransform& transform = registry.emplace<UITransform>(element.entityId);
        transform.sortData.entId = element.entityId;
        transform.sortData.type = element.type;
        transform.asObject = element.asObject;

        registry.emplace<UIVisible>(element.entityId);
        registry.emplace<UIVisibility>(element.entityId);

        switch (element.type)
        {
        case UI::UIElementType::UITYPE_TEXT:
            registry.emplace<UIText>(element.entityId);
            break;
        case UI::UIElementType::UITYPE_PANEL:
            registry.emplace<UIImage>(element.entityId);
            break;
        case UI::UIElementType::UITYPE_INPUTFIELD:
        {
            registry.emplace<UIText>(element.entityId);
            UIInputField& inputField = registry.emplace<UIInputField>(element.entityId);
            inputField.asObject = element.asObject;
            break;
        }
        default:
            break;
        }

        if (registry.any<UIImage, UIText>(element.entityId))
        {
            registry.emplace<UIRenderable>(element.entityId);
        }

        if (element.type != UI::UIElementType::UITYPE_TEXT)
        {
            registry.emplace<UICollision>(element.entityId);
            UITransformEvents& events = registry.emplace<UITransformEvents>(element.entityId);
            events.asObject = element.asObject;
        }
    }
}