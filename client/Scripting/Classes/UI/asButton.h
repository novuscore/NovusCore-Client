#pragma once
#include <NovusTypes.h>
#include <entt.hpp>

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

        //Button Functions.
        const bool IsClickable() const;
        void SetOnClickCallback(asIScriptFunction* callback);

        //Label Functions
        void SetText(const std::string& text);
        const std::string& GetText() const;

        void SetTextColor(const Color& color);
        const Color& GetTextColor() const;

        void SetTextOutlineColor(const Color& outlineColor);
        const Color& GetTextOutlineColor() const;

        void SetTextOutlineWidth(f32 outlineWidth);
        const f32 GetTextOutlineWidth() const;

        void SetTextFont(std::string fontPath, f32 fontSize);
    
        //Panel Functions

        static asButton* CreateButton();

    private:
        asLabel* _label;
        asPanel* _panel;
    };
}