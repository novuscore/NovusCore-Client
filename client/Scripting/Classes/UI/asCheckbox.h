#pragma once
#include "../../../ECS/Components/UI/UITransform.h"
#include "../../../ECS/Components/UI/UITransformEvents.h"
#include "../../../ECS/Components/UI/UIImage.h"
#include "../../../ECS/Components/UI/UICheckbox.h"
#include "asUITransform.h"

namespace UI
{
    class asPanel;

    class asCheckbox : public asUITransform
    {
    public:
        asCheckbox(entt::entity entityId);

        static void RegisterType();

        // TransformEvents Functions
        void SetEventFlag(const UITransformEventsFlags flags) { _events.SetFlag(flags); }
        void UnsetEventFlag(const UITransformEventsFlags flags) { _events.UnsetFlag(flags); }
        const bool IsClickable() const { return _events.IsClickable(); }
        const bool IsDraggable() const { return _events.IsDraggable(); }
        const bool IsFocusable() const { return _events.IsFocusable(); }
        void SetOnClickCallback(asIScriptFunction* callback);
        void SetOnDragCallback(asIScriptFunction* callback);
        void SetOnFocusCallback(asIScriptFunction* callback);

        // Background Functions
        void SetBackgroundTexture(const std::string& texture);
        const std::string& GetBackgroundTexture() const { return _image.texture; }

        void SetBackgroundColor(const Color& color);
        const Color GetBackgroundColor() const { return _image.color; }

        // Check Functions
        void SetCheckTexture(const std::string& texture);
        const std::string& GetCheckTexture() const;

        void SetCheckColor(const Color& color);
        const Color GetCheckColor() const;

        // Checkbox Functions
        void SetChecked(bool checked);
        bool IsChecked() { return _checkBox.checked; }

        static asCheckbox* CreateCheckbox();

    private:
        UITransformEvents _events;
        UICheckbox _checkBox;
        UIImage _image;

        asPanel* checkPanel;
    };
}