#include "TransformUtils.h"

#include "../../Utils/ServiceLocator.h"
#include "../../render-lib/Window/Window.h"
#include "GLFW/glfw3.h"
#include <tracy/Tracy.hpp>

#include "entity/registry.hpp"
#include "../ECS/Components/Singletons/UIDataSingleton.h"

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

    void UpdateChildTransforms(entt::registry* registry, UIComponent::Transform* parent)
    {
        ZoneScoped;
        for (const UI::UIChild& child : parent->children)
        {
            UIComponent::Transform* childTransform = &registry->get<UIComponent::Transform>(child.entId);

            childTransform->anchorPosition = UIUtils::Transform::GetAnchorPositionInElement(parent, childTransform->anchor);
            if (childTransform->HasFlag(UI::TransformFlags::FILL_PARENTSIZE))
                childTransform->size = parent->size;

            UpdateChildTransforms(registry, childTransform);
        }
    }

    void MarkChildrenDirty(entt::registry* registry, const entt::entity entityId)
    {
        ZoneScoped;
        const auto transform = &registry->get<UIComponent::Transform>(entityId);
        for (const UI::UIChild& child : transform->children)
        {
            if (!registry->has<UIComponent::Dirty>(child.entId))
                registry->emplace<UIComponent::Dirty>(child.entId);

            MarkChildrenDirty(registry, child.entId);
        }
    }
}