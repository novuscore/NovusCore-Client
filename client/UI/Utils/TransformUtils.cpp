#include "TransformUtils.h"

#include "../../Utils/ServiceLocator.h"
#include "../../render-lib/Window/Window.h"
#include "GLFW/glfw3.h"
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"

#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "../ECS/Components/Relation.h"

namespace UIUtils::Transform
{
    const hvec2 WindowPositionToUIPosition(const hvec2& WindowPosition)
    {
        i32 width, height;
        glfwGetWindowSize(ServiceLocator::GetWindow()->GetWindow(), &width, &height);
        
        const hvec2 clampedPosition = WindowPosition / hvec2(static_cast<f32>(width), static_cast<f32>(height));
        const hvec2& uiResolution = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>().UIRESOLUTION;

        return clampedPosition * uiResolution;
    }

    hvec2 GetAnchorPositionOnScreen(hvec2 anchorPosition)
    {
        const hvec2& uiResolution = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>().UIRESOLUTION;
        return uiResolution * anchorPosition;
    }

    void UpdateChildTransforms(entt::registry* registry, entt::entity entity)
    {
        auto [transform, relation] = registry->get<UIComponent::Transform, UIComponent::Relation>(entity);

        if (!relation.children.size())
            return;
        
        const hvec2 minAnchorBound = GetMinBounds(transform) + hvec2(transform.padding.left, transform.padding.top);
        const hvec2 innerBounds = GetInnerSize(&transform);

        for (const entt::entity childId : relation.children)
        {
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(childId);

            childTransform->anchorPosition = minAnchorBound + innerBounds * childTransform->anchor;
            if (childTransform->HasFlag(UI::TransformFlags::FILL_PARENTSIZE_X))
                childTransform->size.x = innerBounds.x;
            if (childTransform->HasFlag(UI::TransformFlags::FILL_PARENTSIZE_Y))
                childTransform->size.y = innerBounds.y;

            UpdateChildTransforms(registry, childId);
        }
    }

    void UpdateChildPositions(entt::registry* registry, entt::entity entity)
    {
        auto [transform, relation] = registry->get<UIComponent::Transform, UIComponent::Relation>(entity);

        if (!relation.children.size())
            return;

        const hvec2 minAnchorBound = GetMinBounds(transform) + hvec2(transform.padding.left, transform.padding.top);
        const hvec2 adjustedSize = transform.size - hvec2(transform.padding.right, transform.padding.bottom);

        for (const entt::entity childId : relation.children)
        {
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(childId);
            childTransform->anchorPosition = minAnchorBound + adjustedSize * childTransform->anchor;
        }

        for (const entt::entity childId : relation.children)
        {
            UpdateChildPositions(registry, childId);
        }
    }
}