#pragma once
#include <NovusTypes.h>
#include <entt.hpp>

#include "../../../ECS/Components/UI/UITransform.h"
#include "../../../ECS/Components/UI/UITransformEvents.h"
#include "../../../ECS/Components/UI/UIText.h"

namespace UI
{
    class asLabel
    {
    public:
        static void RegisterType();

        // Transform Functions
        const vec2 GetPosition() const
        {
            return _transform.position;
        }
        void SetPosition(const vec2& position);
        const vec2 GetLocalPosition() const
        {
            return _transform.localPosition;
        }
        void SetLocalPosition(const vec2& localPosition);
        const vec2 GetAnchor() const
        {
            return _transform.anchor;
        }
        void SetAnchor(const vec2& anchor);
        const vec2 GetSize() const
        {
            return _transform.size;
        }
        void SetSize(const vec2& size);
        const u16 GetDepth() const
        {
            return _transform.depth;
        }
        void SetDepth(const u16& depth);

        //Text Functions
        void SetText(const std::string& text);
        const std::string& GetText() const { return _text.text; }

        void SetColor(const Color& color);
        const Color& GetColor() const { return _text.color; }

        void SetOutlineColor(const Color& outlineColor);
        const Color& GetOutlineColor() const { return _text.outlineColor; }

        void SetOutlineWidth(f32 outlineWidth);
        const f32 GetOutlineWidth() const { return _text.outlineWidth; }

        void SetFont(std::string fontPath, f32 fontSize);
    private:
        static asLabel* CreateLabel();

    private:
        entt::entity _entityId;
        UITransform _transform;
        UITransformEvents _events;
        UIText _text;
    };
}