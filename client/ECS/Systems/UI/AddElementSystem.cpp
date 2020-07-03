#include "AddElementSystem.h"
#include <entt.hpp>
#include <tracy/Tracy.hpp>
#include "../../Components/UI/Singletons/UIAddElementQueueSingleton.h"
#include "../../Components/UI/UITransform.h"
#include "../../Components/UI/UITransformEvents.h"
#include "../../Components/UI/UIRenderable.h"
#include "../../Components/UI/UIVisible.h"
#include "../../Components/UI/UIVisiblity.h"
#include "../../Components/UI/UIText.h"
#include "../../Components/UI/UIInputField.h"

void AddElementSystem::Update(entt::registry& registry)
{
    UI::UIAddElementQueueSingleton& uiAddElementQueueSingleton = registry.ctx<UI::UIAddElementQueueSingleton>();

    ZoneScopedNC("AddElementSystem::Update", tracy::Color::Blue)

    UIElementData element;
    while (uiAddElementQueueSingleton.elementPool.try_dequeue(element))
    {
        UITransform& transform = registry.emplace<UITransform>(element.entityId);
        transform.type = element.type;
        transform.asObject = element.asObject;

        registry.emplace<UIVisible>(element.entityId);
        registry.emplace<UIVisiblity>(element.entityId);

        switch (element.type)
        {
        case UIElementType::UITYPE_TEXT:
            registry.emplace<UIText>(element.entityId);
            break;
        case UIElementType::UITYPE_PANEL:
            registry.emplace<UIRenderable>(element.entityId);
            break;
        case UIElementType::UITYPE_INPUTFIELD:
        {
            registry.emplace<UIText>(element.entityId);
            UIInputField& inputField = registry.emplace<UIInputField>(element.entityId);
            inputField.asObject = element.asObject;
            break;
        }
        default:
            break;
        }

        if (element.type != UIElementType::UITYPE_TEXT)
        {
            UITransformEvents& events = registry.emplace<UITransformEvents>(element.entityId);
            events.asObject = element.asObject;
        }
    }
}