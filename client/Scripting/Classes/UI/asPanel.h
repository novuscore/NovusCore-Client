#pragma once
#include <NovusTypes.h>
#include <entt.hpp>

#include "../../../ECS/Components/UI/UITransform.h"
#include "../../../ECS/Components/UI/UITransformEvents.h"
#include "../../../ECS/Components/UI/UIRenderable.h"

namespace UI
{
    class asPanel
    {
    public:
        static void RegisterType();

        // Transform Functions
        const vec2 GetPosition() const
        {
            return transform.position;
        }
        void SetPosition(const vec2& position);
        const vec2 GetLocalPosition() const
        {
            return transform.localPosition;
        }
        void SetLocalPosition(const vec2& localPosition);
        const vec2 GetAnchor() const
        {
            return transform.anchor;
        }
        void SetAnchor(const vec2& anchor);
        const vec2 GetSize() const
        {
            return transform.size;
        }
        void SetSize(const vec2& size);
        const u16 GetDepth() const
        {
            return transform.depth;
        }
        void SetDepth(const u16& depth);

        // TransformEvents Functions
        void SetEventFlag(const UITransformEventsFlags flags) { events.SetFlag(flags); }
        void UnsetEventFlag(const UITransformEventsFlags flags) { events.UnsetFlag(flags); }
        const bool IsClickable() const { return events.IsClickable(); }
        const bool IsDraggable() const { return events.IsDraggable(); }
        const bool IsFocusable() const { return events.IsFocusable(); }
        void SetOnClickCallback(asIScriptFunction* callback);
        void SetOnDragCallback(asIScriptFunction* callback);
        void SetOnFocusCallback(asIScriptFunction* callback);

        // Renderable Functions
        const std::string& GetTexture() const
        {
            return renderable.texture;
        }
        void SetTexture(const std::string& texture);
        const Color GetColor() const
        {
            return renderable.color;
        }
        void SetColor(const Color& color);

    private:
        static asPanel* CreatePanel();

    private:
        entt::entity entityId;
        UITransform transform;
        UITransformEvents events;
        UIRenderable renderable;
    };
}