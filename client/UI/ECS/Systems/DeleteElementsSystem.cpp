#include "DeleteElementsSystem.h"
#include <entity/registry.hpp>

#include "../Components/Singletons/UIDataSingleton.h"
#include "../Components/Destroy.h"
#include "../../angelscript/BaseElement.h"

namespace UISystem
{
    void DeleteElementsSystem::Update(entt::registry& registry)
    {
        auto& dataSingleton = registry.ctx<UISingleton::UIDataSingleton>();

        // Destroy elements queued for destruction.
        auto deleteView = registry.view<UIComponent::Destroy>();
        deleteView.each([&](entt::entity entityId) 
        {
            delete dataSingleton.entityToElement[entityId];
            dataSingleton.entityToElement.erase(entityId);
        });
        registry.destroy(deleteView.begin(), deleteView.end());

    }
}