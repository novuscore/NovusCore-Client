#pragma once
#include <NovusTypes.h>

#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/Image.h"
#include "BaseElement.h"

namespace UIScripting
{
    class Panel : public BaseElement
    {
    public:
        Panel();

        static void RegisterType();

        // TransformEvents Functions
        void SetEventFlag(const UI::UITransformEventsFlags flags) { _events->SetFlag(flags); }
        void UnsetEventFlag(const UI::UITransformEventsFlags flags) { _events->UnsetFlag(flags); }
        const bool IsClickable() const { return _events->IsClickable(); }
        const bool IsDraggable() const { return _events->IsDraggable(); }
        const bool IsFocusable() const { return _events->IsFocusable(); }
        void SetOnClickCallback(asIScriptFunction* callback);
        void SetOnDragCallback(asIScriptFunction* callback);
        void SetOnFocusCallback(asIScriptFunction* callback);

        // Renderable Functions
        void SetTexture(const std::string& texture);
        const std::string& GetTexture() const { return _image->texture; }

        void SetColor(const Color& color);
        const Color GetColor() const { return _image->color; }

        static Panel* CreatePanel();

    private:
        UIComponent::TransformEvents* _events = nullptr;
        UIComponent::Image* _image = nullptr;
    };
}