#include "TransformUtils.h"

#include "../../Utils/ServiceLocator.h"
#include "../../render-lib/Window/Window.h"
#include "GLFW/glfw3.h"
#include <tracy/Tracy.hpp>
#include "entity/registry.hpp"

#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "../ECS/Components/Relation.h"
#include "../ECS/Components/TransformFill.h"

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
            UIComponent::Transform& childTransform = registry->get<UIComponent::Transform>(childId);

            childTransform.anchorPosition = minAnchorBound + innerBounds * childTransform.anchor;

            if (registry->has<UIComponent::TransformFill>(childId))
            {
                //Calculate size & position for elements that want to fill.
                const UIComponent::TransformFill& childTransformFill = registry->get<UIComponent::TransformFill>(childId);
                CalculateFillFromInnerBounds(childTransformFill, innerBounds, childTransform.position, childTransform.size);
            }
        }

        for (const entt::entity childId : relation.children)
        {
            UpdateChildTransforms(registry, childId);
        }
    }

    void CalculateFillFromInnerBounds(const UIComponent::TransformFill& childTransformFill, const hvec2 innerBounds, hvec2& selfPosition, hvec2& selfSize)
    {
        if (childTransformFill.HasFlag(UI::TransformFillFlags::FILL_PARENTSIZE_X))
        {
            selfPosition.x = childTransformFill.fill.left * innerBounds.x;
            selfSize.x = childTransformFill.fill.right * innerBounds.x - selfPosition.x;
        }
        if (childTransformFill.HasFlag(UI::TransformFillFlags::FILL_PARENTSIZE_Y))
        {
            selfPosition.y = childTransformFill.fill.top * innerBounds.y;
            selfSize.y = childTransformFill.fill.bottom * innerBounds.y - selfPosition.y;
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