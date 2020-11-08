#pragma once
#include <NovusTypes.h>
#include "../ECS/Components/Text.h"

#include "BaseElement.h"

namespace UIScripting
{
    class Label;
    class Panel;

    class InputField : public BaseElement
    {
    public:
        InputField();

        static void RegisterType();

        void HandleKeyInput(i32 key);

        //InputField Functions
        void HandleCharInput(const char input);

        void RemovePreviousCharacter();
        void RemoveNextCharacter();

        void MovePointerLeft();
        void MovePointerRight();

        void SetWriteHeadPosition(size_t position);

        void SetOnSubmitCallback(asIScriptFunction* callback);

        // TransformEvents Functions
        const bool IsFocusable() const;
        void SetFocusable(bool focusable);
        void SetOnFocusGainedCallback(asIScriptFunction* callback);
        void SetOnFocusLostCallback(asIScriptFunction* callback);

        //Label Functions
        const std::string GetText() const;
        void SetText(const std::string& newText, bool updateWriteHead = true);

        const Color& GetColor() const;
        void SetColor(const Color& color);

        const Color& GetOutlineColor() const;
        void SetOutlineColor(const Color& outlineColor);

        const f32 GetOutlineWidth() const;
        void SetOutlineWidth(f32 outlineWidth);

        void SetFont(const std::string& fontPath, f32 fontSize);

        void SetHorizontalAlignment(UI::TextHorizontalAlignment alignment);
        void SetVerticalAlignment(UI::TextVerticalAlignment alignment);

        static InputField* CreateInputField();
    };
}