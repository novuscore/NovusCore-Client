#include "UpdateBoundsSystem.h"
#include <entity/registry.hpp>

#include "../Components/Transform.h"
#include "../Components/Relation.h"
#include "../Components/Collision.h"
#include "../Components/BoundsDirty.h"
#include "../../Utils/ColllisionUtils.h"


namespace UISystem
{
    void UpdateBoundsSystem::Update(entt::registry& registry)
    {
        auto boundsUpdateView = registry.view<UIComponent::Transform, UIComponent::Collision, UIComponent::Relation, UIComponent::BoundsDirty>();
        boundsUpdateView.each([&](entt::entity entityId, UIComponent::Transform& transform, UIComponent::Collision& collision, UIComponent::Relation& relation)
        {
            UIUtils::Collision::UpdateBounds(&registry, entityId, true);
        });
    }
}