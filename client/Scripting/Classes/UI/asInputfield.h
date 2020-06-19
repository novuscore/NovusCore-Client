#pragma once
#include <NovusTypes.h>
#include <entt.hpp>

#include "asUITransform.h"

#include "../../../ECS/Components/UI/UITransformEvents.h"
#include "../../../ECS/Components/UI/UIInputField.h"
#include "../../../ECS/Components/UI/UIText.h"

namespace UI
{
    class asLabel;
    class asPanel;

    class asInputField : public asUITransform
    {
    public:
        asInputField(entt::entity entityId);

        static void RegisterType();

        //InputField Functions
        void AppendInput(const std::string& input);
        void AppendInput(const char input) 
        {
            std::string strInput = "";
            strInput.append(1, input);
            AppendInput(strInput);
        }

        void RemovePreviousCharacter();
        void RemoveNextCharacter();

        void MovePointerLeft();
        void MovePointerRight();

        void SetWriteHeadPosition(u32 position);

        // TransformEvents Functions
        const bool IsFocusable() const { return _events.IsFocusable(); }
        void SetOnFocusCallback(asIScriptFunction* callback);
        void SetOnUnFocusCallback(asIScriptFunction* callback);

        //Label Functions
        void SetText(const std::string& text, bool updateWriteHead = true);
        const std::string& GetText() const;

        void SetTextColor(const Color& color);
        const Color& GetTextColor() const;

        void SetTextOutlineColor(const Color& outlineColor);
        const Color& GetTextOutlineColor() const;

        void SetTextOutlineWidth(f32 outlineWidth);
        const f32 GetTextOutlineWidth() const;

        void SetTextFont(std::string fontPath, f32 fontSize);

        static asInputField* CreateInputField();

    private:
        UITransformEvents _events;
        UIText _text;
        UIInputField _inputField;
    };
}