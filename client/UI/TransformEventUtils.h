#pragma once
#include <NovusTypes.h>
#include <entity/entity.hpp>
#include <entity/registry.hpp>
#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/ScriptSingleton.h"
#include "../ECS/Components/UI/UITransformEvents.h"

namespace UI::TransformEventUtils
{
    inline static void SetFlag(entt::entity entId, UITransformEventsFlags flag)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, flag]()
            {
                UITransformEvents& events = ServiceLocator::GetUIRegistry()->get<UITransformEvents>(entId);

                events.SetFlag(flag);
            });
    }

    inline static void UnsetFlag(entt::entity entId, UITransformEventsFlags flag)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, flag]()
            {
                UITransformEvents& events = ServiceLocator::GetUIRegistry()->get<UITransformEvents>(entId);

                events.UnsetFlag(flag);
            });
    }

    inline static void SetOnClickCallback(entt::entity entId, asIScriptFunction* callback)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, callback]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);

                events.onClickCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);
            });
    }

    inline static void SetOnDragCallback(entt::entity entId, asIScriptFunction* callback)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, callback]()
            {
                UITransformEvents& events = ServiceLocator::GetUIRegistry()->get<UITransformEvents>(entId);

                events.onDraggedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_DRAGGABLE);
            });
    }

    inline static void SetOnFocusCallback(entt::entity entId, asIScriptFunction* callback)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, callback]()
            {
                UITransformEvents& events = ServiceLocator::GetUIRegistry()->get<UITransformEvents>(entId);

                events.onFocusedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });
    }
};