#pragma once
#include <NovusTypes.h>

#include "BaseElement.h"

#include "../ECS/Components/Transform.h"
#include "../ECS/Components/TransformEvents.h"

namespace UIScripting
{
    class Label;
    class Panel;

    class Button : public BaseElement
    {
    public:
        Button();

        static void RegisterType();

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

        static Button* CreateButton();

    private:
        Label* _label;
        Panel* _panel;

        UIComponent::TransformEvents _events;
    };
}