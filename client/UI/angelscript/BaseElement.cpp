#include "BaseElement.h"
#include <tracy/Tracy.hpp>
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "../ECS/Components/Singletons/UIEntityPoolSingleton.h"
#include "../ECS/Components/Singletons/UILockSingleton.h"

#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Collidable.h"
#include "../ECS/Components/Dirty.h"
#include "../ECS/Components/BoundsDirty.h"
#include "../Utils/TransformUtils.h"
#include "../Utils/VisibilityUtils.h"

namespace UIScripting
{
    BaseElement::BaseElement(UI::UIElementType elementType) : _elementType(elementType)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        _entityId = registry->ctx<UISingleton::UIEntityPoolSingleton>().GetId();
        registry->ctx<UISingleton::UIDataSingleton>().entityToAsObject[_entityId] = this;
    }

    vec2 BaseElement::GetScreenPosition() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return UIUtils::Transform::GetScreenPosition(transform);
    }
    vec2 BaseElement::GetLocalPosition() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->parent == entt::null ? vec2(0, 0) : transform->localPosition;
    }
    vec2 BaseElement::GetParentPosition() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->parent == entt::null ? vec2(0, 0) : transform->position;
    }
    void BaseElement::SetPosition(const vec2& position)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->parent == entt::null)
            transform->position = position;
        else
            transform->localPosition = position;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
    }

    vec2 BaseElement::GetSize() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->size;
    }
    void BaseElement::SetSize(const vec2& size)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        // Early out if we are just filling parent size.
        if (transform->fillParentSize)
            return;
        transform->size = size;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
    }

    void BaseElement::SetTransform(const vec2& position, const vec2& size)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->parent == entt::null)
            transform->position = position;
        else
            transform->localPosition = position;

        if (!transform->fillParentSize)
            transform->size = size;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
    }

    vec2 BaseElement::GetAnchor() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->anchor;
    }
    void BaseElement::SetAnchor(const vec2& anchor)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->anchor == anchor)
            return;
        transform->anchor = anchor;

        if (transform->parent != entt::null)
        {
            auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();
            std::shared_lock pl(dataSingleton->GetMutex(transform->parent));

            transform->position = UIUtils::Transform::GetAnchorPosition(&registry->get<UIComponent::Transform>(transform->parent), anchor);
        }

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
    }

    vec2 BaseElement::GetLocalAnchor() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->localAnchor;
    }
    void BaseElement::SetLocalAnchor(const vec2& localAnchor)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->localAnchor == localAnchor)
            return;
        transform->localAnchor = localAnchor;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
    }

    bool BaseElement::GetFillParentSize() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->fillParentSize;
    }
    void BaseElement::SetFillParentSize(bool fillParent)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->fillParentSize == fillParent)
            return;
        transform->fillParentSize = fillParent;

        if (transform->parent == entt::null)
            return;

        auto parentTransform = &registry->get<UIComponent::Transform>(transform->parent);
        transform->size = parentTransform->size;

        UIUtils::Transform::UpdateChildTransforms(registry, transform);
    }

    UI::DepthLayer BaseElement::GetDepthLayer() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->sortData.depthLayer;
    }
    void BaseElement::SetDepthLayer(const UI::DepthLayer layer)
    {
        auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);

        transform->sortData.depthLayer = layer;
    }

    u16 BaseElement::GetDepth() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->sortData.depth;
    }
    void BaseElement::SetDepth(const u16 depth)
    {
        auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);

        transform->sortData.depth = depth;
    }

    void BaseElement::SetParent(BaseElement* parent)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->parent == parent->GetEntityId())
            return;

        if (transform->parent != entt::null)
        {
            auto oldParentTransform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(transform->parent);
            UIUtils::Transform::RemoveChild(oldParentTransform, transform);
        }
        transform->parent = parent->GetEntityId();
        
        auto parentTransform = &registry->get<UIComponent::Transform>(transform->parent);
        // Add us as parent's child.
        parentTransform->children.push_back({ _entityId, _elementType });

        // Update position. Keeping relative.
        vec2 Origin = UIUtils::Transform::GetAnchorPosition(parentTransform, transform->anchor);
        transform->localPosition = transform->position - Origin;
        transform->position = Origin;

        // Handle fillParentSize
        if (transform->fillParentSize)
            transform->size = parentTransform->size;

        // Update our and children's depth. Keeping the relative offsets for all children but adding onto it how much we moved in depth.
        u16 difference = parentTransform->sortData.depth - transform->sortData.depth + 1;
        transform->sortData.depth = parentTransform->sortData.depth + 1;
        UIUtils::Transform::UpdateChildDepths(registry, transform, difference);
        UIUtils::Transform::UpdateChildTransforms(registry, transform);
    }
    void BaseElement::UnsetParent()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->parent == entt::null)
            return;

        auto parentTransform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        UIUtils::Transform::RemoveChild(parentTransform, transform);
    }

    bool BaseElement::GetExpandBoundsToChildren() const
    {
        const UIComponent::Transform* transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->includeChildBounds;
    }
    void BaseElement::SetExpandBoundsToChildren(bool expand)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->includeChildBounds == expand)
            return;
        transform->includeChildBounds = expand;
    }

    bool BaseElement::IsVisible() const
    {
        const UIComponent::Visibility* visibility = &ServiceLocator::GetUIRegistry()->get<UIComponent::Visibility>(_entityId);
        return UIUtils::Visibility::IsVisible(visibility);
    }
    bool BaseElement::IsLocallyVisible() const
    {
        const UIComponent::Visibility* visibility = &ServiceLocator::GetUIRegistry()->get<UIComponent::Visibility>(_entityId);
        return visibility->visible;
    }
    bool BaseElement::IsParentVisible() const
    {
        const UIComponent::Visibility* visibility = &ServiceLocator::GetUIRegistry()->get<UIComponent::Visibility>(_entityId);
        return visibility->parentVisible;
    }
    void BaseElement::SetVisible(bool visible)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto visibility = &registry->get<UIComponent::Visibility>(_entityId);

        if (visibility->visible == visible)
            return;
        const bool visibilityChanged = (visibility->parentVisible && visibility->visible) != (visibility->parentVisible && visible);
        visibility->visible = visible;
        
        if (!visibilityChanged)
            return;

        const bool newVisibility = UIUtils::Visibility::IsVisible(visibility);
        UIUtils::Visibility::UpdateChildVisibility(registry, _entityId, newVisibility);

        registry->ctx<UISingleton::UIDataSingleton>().visibilityToggleQueue.enqueue(_entityId);
    }

    void BaseElement::SetCollisionEnabled(bool enabled)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();

        if (registry->has<UIComponent::Collidable>(_entityId) == enabled)
            return;

        registry->ctx<UISingleton::UIDataSingleton>().collisionToggleQueue.enqueue(_entityId);
    }

    void BaseElement::Destroy()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        registry->ctx<UISingleton::UIDataSingleton>().DestroyWidget(_entityId);
    }

    void BaseElement::MarkDirty()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        if (!registry->has<UIComponent::Dirty>(_entityId))
            registry->emplace<UIComponent::Dirty>(_entityId);

        const auto transform = &registry->get<UIComponent::Transform>(_entityId);
        UIUtils::Transform::MarkChildrenDirty(registry, transform);
    }

    void BaseElement::MarkSelfDirty()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        if (!registry->has<UIComponent::Dirty>(_entityId))
            registry->emplace<UIComponent::Dirty>(_entityId);
    }

    void BaseElement::MarkBoundsDirty()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        if (!registry->has<UIComponent::BoundsDirty>(_entityId))
            registry->emplace<UIComponent::BoundsDirty>(_entityId);
    }
}
