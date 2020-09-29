#include "ElementUtils.h"
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Destroy.h"
#include <entity/registry.hpp>
#include <tracy/Tracy.hpp>

namespace UIUtils
{
    void MarkChildrenForDestruction(entt::registry* registry, entt::entity entityId)
    {
        const UIComponent::Transform* transform = &registry->get<UIComponent::Transform>(entityId);
        for (const UI::UIChild& child : transform->children)
        {
            if (!registry->has<UIComponent::Destroy>(child.entId))
                registry->emplace<UIComponent::Destroy>(child.entId);

            MarkChildrenForDestruction(registry, entityId);
        }
    }
}