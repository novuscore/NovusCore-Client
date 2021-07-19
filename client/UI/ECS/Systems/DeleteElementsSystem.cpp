#include "DeleteElementsSystem.h"
#include <entity/registry.hpp>

#include "../Components/Singletons/UIDataSingleton.h"
#include "../Components/Destroy.h"
#include "../Components/Relation.h"
#include "../../angelscript/BaseElement.h"
#include "../../Utils/ElementUtils.h"

namespace UISystem
{
    void DeleteElementsSystem::Update(entt::registry& registry)
    {
        auto& dataSingleton = registry.ctx<UISingleton::UIDataSingleton>();

        // Destroy elements queued for destruction.
        auto deleteView = registry.view<UIComponent::Relation, UIComponent::Destroy>();
        deleteView.each([&](entt::entity entityId, UIComponent::Relation& relation) 
        {
            delete dataSingleton.entityToElement[entityId];
            dataSingleton.entityToElement.erase(entityId);

            if (relation.parent != entt::null)
            {
                UIComponent::Relation& parentRelation = registry.get<UIComponent::Relation>(relation.parent);
                parentRelation.children.erase(std::remove(parentRelation.children.begin(), parentRelation.children.end(), entityId), parentRelation.children.end());
            }

            for (entt::entity childId : relation.children)
            {
                if(!registry.has<UIComponent::Destroy>(childId))
                    UIUtils::RemoveFromParent(&registry, childId);
            }
        });
        registry.destroy(deleteView.begin(), deleteView.end());

    }
}