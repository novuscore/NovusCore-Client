#pragma once
#include <NovusTypes.h>
#include "BaseElement.h"

namespace UIScripting
{
    class Panel : public BaseElement
    {
    public:
        Panel(bool collisionEnabled = true);

        static void RegisterType();

        // TransformEvents Functions
        const bool IsClickable() const;
        void SetClickable(bool clickable);
        const bool IsDraggable() const;
        void SetDraggable(bool draggable);
        const bool IsFocusable() const;
        void SetFocusable(bool focusable);
        
        void SetOnClickCallback(asIScriptFunction* callback);
        
        void SetOnDragStartedCallback(asIScriptFunction* callback);
        void SetOnDragEndedCallback(asIScriptFunction* callback);

        void SetOnFocusGainedCallback(asIScriptFunction* callback);
        void SetOnFocusLostCallback(asIScriptFunction* callback);

        // Renderable Functions
        const std::string& GetTexture() const;
        void SetTexture(const std::string& texture);

        void SetTexCoord(const vec4& texCoords);

        const Color GetColor() const;
        void SetColor(const Color& color);

        const std::string& GetBorder() const;
        void SetBorder(const std::string& texture);
        void SetBorderSize(const u32 topSize, const u32 rightSize, const u32 bottomSize, const u32 leftSize);
        void SetBorderInset(const u32 topBorderInset, const u32 rightBorderInset, const u32 bottomBorderInset, const u32 leftBorderInset);
        void SetSlicing(const u32 topOffset, const u32 rightOffset, const u32 bottomOffset, const u32 leftOffset);

        static Panel* CreatePanel(bool collisionEnabled = true);
    };
}