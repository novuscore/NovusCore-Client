#pragma once
#include <NovusTypes.h>
#include "BaseElement.h"

#include "../ECS/Components/Image.h"

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
        void SetStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetFocusedStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetHoverStylesheet(const UI::ImageStylesheet& styleSheet);
        void SetPressedStylesheet(const UI::ImageStylesheet& styleSheet);

        static Panel* CreatePanel(bool collisionEnabled = true);
    };
}