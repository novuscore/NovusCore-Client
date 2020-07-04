#pragma once
#include <NovusTypes.h>

#include "../../../ECS/Components/UI/UITransform.h"
#include "../../../ECS/Components/UI/UITransformEvents.h"
#include "asUITransform.h"

namespace UI
{
    class asLabel;
    class asPanel;

    class asButton : public asUITransform
    {
    public:
        asButton(entt::entity entityId);

        static void RegisterType();

        //Transform Functions.
        virtual void SetSize(const vec2& size);

        //Button Functions.
        const bool IsClickable() const { return _events.IsClickable(); }
        void SetOnClickCallback(asIScriptFunction* callback);

        //Label Functions
        void SetText(const std::string& text);
        const std::string GetText() const;

        void SetTextColor(const Color& color);
        const Color& GetTextColor() const;

        void SetTextOutlineColor(const Color& outlineColor);
        const Color& GetTextOutlineColor() const;

        void SetTextOutlineWidth(f32 outlineWidth);
        const f32 GetTextOutlineWidth() const;

        void SetTextFont(std::string fontPath, f32 fontSize);
    
        //Panel Functions        
        void SetTexture(const std::string& texture);
        const std::string& GetTexture() const;

        void SetColor(const Color& color);
        const Color GetColor() const;

        static asButton* CreateButton();

    private:
        asLabel* _label;
        asPanel* _panel;

        UITransformEvents _events;
    };
}