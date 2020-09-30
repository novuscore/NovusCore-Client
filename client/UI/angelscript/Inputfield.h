#pragma once
#include <NovusTypes.h>

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
        void SetOnFocusCallback(asIScriptFunction* callback);
        void SetOnUnFocusCallback(asIScriptFunction* callback);

        //Label Functions
        const std::string GetText() const;
        void SetText(const std::string& newText, bool updateWriteHead = true);

        const Color& GetTextColor() const;
        void SetTextColor(const Color& color);

        const Color& GetTextOutlineColor() const;
        void SetTextOutlineColor(const Color& outlineColor);

        const f32 GetTextOutlineWidth() const;
        void SetTextOutlineWidth(f32 outlineWidth);

        void SetTextFont(const std::string& fontPath, f32 fontSize);

        static InputField* CreateInputField();
    };
}