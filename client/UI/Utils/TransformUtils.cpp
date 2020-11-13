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
        ZoneScoped;
        for (const UI::UIChild& child : relation.children)
        {
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);

            childTransform->anchorPosition = UIUtils::Transform::GetAnchorPositionInElement(&transform, childTransform->anchor);
            if (childTransform->HasFlag(UI::TransformFlags::FILL_PARENTSIZE))
                childTransform->size = transform.size;

            UpdateChildTransforms(registry, child.entId);
        }
    }
}