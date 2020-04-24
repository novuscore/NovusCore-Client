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

        vec2 GetPosition();
        void SetPosition(vec2& position);
        vec2 GetLocalPosition();
        void SetLocalPosition(vec2& localPosition);
        vec2 GetAnchor();
        void SetAnchor(vec2& anchor);
        vec2 GetSize();
        void SetSize(vec2& size);
        u16 GetDepth();
        void SetDepth(u16& depth);

    private:
        static asPanel* CreatePanel();

    private:
        entt::entity entityId;
        UITransform transform;
        UITransformEvents events;
        UIRenderable renderable;
    };
}