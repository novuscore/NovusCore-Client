#pragma once
#include <NovusTypes.h>
#include <entt.hpp>

#include "../../../ECS/Components/UI/UITransformEvents.h"
#include "asUITransform.h"

namespace UI
{
    class asLabel;
    class asPanel;

    class asInputfield : public asUITransform
    {
    public:
        asInputfield(entt::entity entityId);

        static void RegisterType();

        //Transform Functions.
        virtual void SetSize(const vec2& size);

        // TransformEvents Functions
        void SetEventFlag(const UITransformEventsFlags flags) { _events.SetFlag(flags); }
        void UnsetEventFlag(const UITransformEventsFlags flags) { _events.UnsetFlag(flags); }
        const bool IsFocusable() const { return _events.IsFocusable(); }
        void SetOnFocusCallback(asIScriptFunction* callback);

        static asInputfield* CreateInputfield();

    private:
        asLabel* _label;
        asPanel* _panel;

        UITransformEvents _events;
    };
}