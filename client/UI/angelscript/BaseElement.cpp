#include "BaseElement.h"
#include <tracy/Tracy.hpp>
#include "../../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "../ECS/Components/ElementInfo.h"
#include "../ECS/Components/Name.h"
#include "../ECS/Components/Root.h"
#include "../ECS/Components/Relation.h"
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/SortKey.h"
#include "../ECS/Components/SortKeyDirty.h"
#include "../ECS/Components/Visibility.h"
#include "../ECS/Components/Visible.h"
#include "../ECS/Components/Collision.h"
#include "../ECS/Components/Collidable.h"
#include "../ECS/Components/Dirty.h"
#include "../ECS/Components/BoundsDirty.h"
#include "../ECS/Components/Destroy.h"

#include "../Utils/ElementUtils.h"
#include "../Utils/TransformUtils.h"
#include "../Utils/SortUtils.h"
#include "../Utils/VisibilityUtils.h"

namespace UIScripting
{
    BaseElement::BaseElement(UI::ElementType elementType, std::string name, bool collisionEnabled) : _elementType(elementType)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        _entityId = registry->create();
        registry->ctx<UISingleton::UIDataSingleton>().entityToElement[_entityId] = this;

        // Set up base components.
        registry->emplace<UIComponent::ElementInfo>(_entityId, elementType, this);
        registry->emplace<UIComponent::Name>(_entityId, name);

        registry->emplace<UIComponent::Root>(_entityId);
        registry->emplace<UIComponent::Relation>(_entityId);
        registry->emplace<UIComponent::Transform>(_entityId);

        registry->emplace<UIComponent::SortKey>(_entityId);
        
        registry->emplace<UIComponent::Visibility>(_entityId);
        registry->emplace<UIComponent::Visible>(_entityId);

        UIComponent::Collision* collision = &registry->emplace<UIComponent::Collision>(_entityId);
        if (collisionEnabled)
        {
            collision->SetFlag(UI::CollisionFlags::COLLISION);
            registry->emplace<UIComponent::Collidable>(_entityId);
        }
    }

    vec2 BaseElement::GetScreenPosition() const
    {
        const UIComponent::Transform& transform = ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return UIUtils::Transform::GetScreenPosition(transform);
    }
    vec2 BaseElement::GetLocalPosition() const
    {
        const UIComponent::Transform& transform = ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform.position;
    }
    void BaseElement::SetPosition(const vec2& position)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform& transform = registry->get<UIComponent::Transform>(_entityId);
        transform.position = position;

        UIUtils::Transform::UpdateChildPositions(registry, _entityId);
    }

    vec2 BaseElement::GetSize() const
    {
        const UIComponent::Transform& transform = ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform.size;
    }
    void BaseElement::SetSize(const vec2& size)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform& transform = registry->get<UIComponent::Transform>(_entityId);

        // Early out if we are just filling parent size.
        if (transform.HasFlag(UI::TransformFlags::FILL_PARENTSIZE))
            return;
        transform.size = size;

        UIUtils::Transform::UpdateChildTransforms(registry, _entityId);
    }
    bool BaseElement::GetFillParentSize() const
    {
        const auto transform = &ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform->HasFlag(UI::TransformFlags::FILL_PARENTSIZE);
    }
    void BaseElement::SetFillParentSize(bool fillParent)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto [transform, relation] = registry->get <UIComponent::Transform, UIComponent::Relation>(_entityId);

        if (transform.HasFlag(UI::TransformFlags::FILL_PARENTSIZE) == fillParent)
            return;
        transform.ToggleFlag(UI::TransformFlags::FILL_PARENTSIZE);

        if (relation.parent == entt::null)
            return;

        auto parentTransform = &registry->get<UIComponent::Transform>(relation.parent);
        transform.size = UIUtils::Transform::GetInnerSize(parentTransform);

        UIUtils::Transform::UpdateChildTransforms(registry, _entityId);
    }

    void BaseElement::SetTransform(const vec2& position, const vec2& size)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Transform& transform = registry->get<UIComponent::Transform>(_entityId);

        transform.position = position;
        if (!transform.HasFlag(UI::TransformFlags::FILL_PARENTSIZE))
            transform.size = size;

        UIUtils::Transform::UpdateChildTransforms(registry, _entityId);
    }

    vec2 BaseElement::GetAnchor() const
    {
        const UIComponent::Transform& transform = ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform.anchor;
    }
    void BaseElement::SetAnchor(const vec2& anchor)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto [transform, relation] = registry->get<UIComponent::Transform, UIComponent::Relation>(_entityId);

        if (transform.anchor == hvec2(anchor))
            return;
        transform.anchor = anchor;

        if (relation.parent == entt::null)
            transform.anchorPosition = UIUtils::Transform::GetAnchorPositionOnScreen(anchor);
        else
            transform.anchorPosition = UIUtils::Transform::GetAnchorPositionInElement(registry->get<UIComponent::Transform>(relation.parent), anchor);

        UIUtils::Transform::UpdateChildPositions(registry, _entityId);
    }

    vec2 BaseElement::GetLocalAnchor() const
    {
        const UIComponent::Transform& transform = ServiceLocator::GetUIRegistry()->get<UIComponent::Transform>(_entityId);
        return transform.localAnchor;
    }
    void BaseElement::SetLocalAnchor(const vec2& localAnchor)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);

        if (transform->localAnchor == hvec2(localAnchor))
            return;
        transform->localAnchor = localAnchor;

        UIUtils::Transform::UpdateChildTransforms(registry, _entityId);
    }

    void BaseElement::SetPadding(f32 top, f32 right, f32 bottom, f32 left)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto transform = &registry->get<UIComponent::Transform>(_entityId);
        transform->padding = UI::HBox{ f16(top), f16(right), f16(bottom), f16(left) };

        UIUtils::Transform::UpdateChildTransforms(registry, _entityId);
    }

    UI::DepthLayer BaseElement::GetDepthLayer() const
    {
        const UIComponent::SortKey& sortKey = ServiceLocator::GetUIRegistry()->get<UIComponent::SortKey>(_entityId);
        return sortKey.data.depthLayer;
    }
    void BaseElement::SetDepthLayer(const UI::DepthLayer layer)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        if (!registry->has<UIComponent::Root>(_entityId))
        {
            DebugHandler::PrintWarning("UI: Can't set depthLayer on non-root element (ID: %u, Type: %u).", entt::to_integral(_entityId), static_cast<u32>(_elementType));
            return;
        }

        auto sortKey = &registry->get<UIComponent::SortKey>(_entityId);
        sortKey->data.depthLayer = layer;

        if (!registry->has<UIComponent::SortKeyDirty>(_entityId))
            registry->emplace<UIComponent::SortKeyDirty>(_entityId);
    }

    u16 BaseElement::GetDepth() const
    {
        const UIComponent::SortKey& sortKey = ServiceLocator::GetUIRegistry()->get<UIComponent::SortKey>(_entityId);
        return sortKey.data.depth;
    }
    void BaseElement::SetDepth(const u16 depth)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry(); 
        if (!registry->has<UIComponent::Root>(_entityId))
        {
            DebugHandler::PrintWarning("UI: Can't set depth on non-root element.");
            return;
        }

        auto sortKey = &registry->get<UIComponent::SortKey>(_entityId);
        sortKey->data.depth = depth;

        if (!registry->has<UIComponent::SortKeyDirty>(_entityId))
            registry->emplace<UIComponent::SortKeyDirty>(_entityId);
    }

    const bool BaseElement::HasParent() const
    {
        const UIComponent::Relation& relation = ServiceLocator::GetUIRegistry()->get<UIComponent::Relation>(_entityId);
        return relation.parent != entt::null;
    }
    BaseElement* BaseElement::GetParent() const
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        const UIComponent::Relation& relation = registry->get<UIComponent::Relation>(_entityId);
        if (relation.parent == entt::null)
            return nullptr;

        return reinterpret_cast<BaseElement*>(registry->get<UIComponent::ElementInfo>(relation.parent).scriptingObject);
    }

    void BaseElement::AddChild(BaseElement* child)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Relation& childRelation = registry->get<UIComponent::Relation>(child->GetEntityId());

        if (childRelation.parent != entt::null)
        {
            DebugHandler::PrintError("Tried calling AddChild() on Element(ID: %d, Type: %s) with a parent. You must call RemoveFromParent() first.", entt::to_integral(child->GetEntityId()), UI::GetElementTypeAsString(child->GetType()));
            return;
        }
        UIUtils::AddChild(registry, _entityId, child->GetEntityId());
    }
    void BaseElement::RemoveChild(BaseElement* child)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIComponent::Relation& childRelation = registry->get<UIComponent::Relation>(child->GetEntityId());

        if (childRelation.parent != _entityId)
        {
            DebugHandler::PrintError("Tried calling RemoveChild() on Element(ID: %d, Type: %s) that is not a child of this element (ID: %d, Type: %s).", entt::to_integral(child->GetEntityId()), UI::GetElementTypeAsString(child->GetType()), entt::to_integral(_entityId), UI::GetElementTypeAsString(_elementType));
            return;
        }

        UIUtils::RemoveFromParent(registry, child->GetEntityId());
    }
    void BaseElement::RemoveFromParent()
    {
        if (BaseElement* parent = GetParent())
            parent->RemoveChild(this);
    }

    bool BaseElement::GetCollisionIncludesChildren() const
    {
        const auto collision = &ServiceLocator::GetUIRegistry()->get<UIComponent::Collision>(_entityId);
        return collision->HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS);
    }
    void BaseElement::SetCollisionIncludesChildren(bool expand)
    {
        auto collision = &ServiceLocator::GetUIRegistry()->get<UIComponent::Collision>(_entityId);

        if (collision->HasFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS) == expand)
            return;

        collision->ToggleFlag(UI::CollisionFlags::INCLUDE_CHILDBOUNDS);
    }

    bool BaseElement::IsVisible() const
    {
        const UIComponent::Visibility* visibility = &ServiceLocator::GetUIRegistry()->get<UIComponent::Visibility>(_entityId);
        return visibility->visibilityFlags == UI::VisibilityFlags::FULL_VISIBLE;
    }
    bool BaseElement::IsSelfVisible() const
    {
        const UIComponent::Visibility* visibility = &ServiceLocator::GetUIRegistry()->get<UIComponent::Visibility>(_entityId);
        return visibility->HasFlag(UI::VisibilityFlags::VISIBLE);
    }
    bool BaseElement::IsParentVisible() const
    {
        const UIComponent::Visibility* visibility = &ServiceLocator::GetUIRegistry()->get<UIComponent::Visibility>(_entityId);
        return visibility->HasFlag(UI::VisibilityFlags::PARENTVISIBLE);
    }
    void BaseElement::SetVisible(bool visible)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto visibility = &registry->get<UIComponent::Visibility>(_entityId);

        if (!UIUtils::Visibility::UpdateVisibility(visibility, visible))
            return;

        const bool newVisibility = UIUtils::Visibility::IsVisible(visibility);
        UIUtils::Visibility::UpdateChildVisibility(registry, _entityId, newVisibility);

        if (newVisibility)
            registry->emplace<UIComponent::Visible>(_entityId);
        else
            registry->remove<UIComponent::Visible>(_entityId);
    }

    void BaseElement::SetCollisionEnabled(bool enabled)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto collision = &registry->get<UIComponent::Collision>(_entityId);
        if (collision->HasFlag(UI::CollisionFlags::COLLISION) == enabled)
            return;

        collision->ToggleFlag(UI::CollisionFlags::COLLISION);

        if (enabled)
            registry->emplace<UIComponent::Collidable>(_entityId);
        else
            registry->remove<UIComponent::Collidable>(_entityId);
    }

    void BaseElement::Destroy(bool destroyChildren)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        if(!registry->has<UIComponent::Destroy>(_entityId))
            registry->emplace<UIComponent::Destroy>(_entityId);

        if (destroyChildren)
            UIUtils::MarkChildrenForDestruction(registry, _entityId);
    }

    void BaseElement::MarkDirty()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        if (!registry->has<UIComponent::Dirty>(_entityId))
            registry->emplace<UIComponent::Dirty>(_entityId);

        UIUtils::MarkChildrenDirty(registry, _entityId);
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

    void BaseElement::InternalAddChild(BaseElement* element)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto [elementRelation, elementSortKey] = registry->get<UIComponent::Relation, UIComponent::SortKey>(element->GetEntityId());
        elementRelation.parent = _entityId;
        elementSortKey.data.depth++;
        registry->remove<UIComponent::Root>(element->GetEntityId());

        registry->get<UIComponent::Relation>(_entityId).children.push_back(element->GetEntityId());
    }
}