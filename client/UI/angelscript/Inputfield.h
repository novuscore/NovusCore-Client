#pragma once
#include <NovusTypes.h>

#include "BaseElement.h"

#include "../ECS/Components/TransformEvents.h"
#include "../ECS/Components/InputField.h"
#include "../ECS/Components/Text.h"

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
        void SetFocusable(bool focusable);
        const bool IsFocusable() const { return _events->IsFocusable(); }
        void SetOnFocusCallback(asIScriptFunction* callback);
        void SetOnUnFocusCallback(asIScriptFunction* callback);

        //Label Functions
        void SetText(const std::string& text, bool updateWriteHead = true);
        const std::string GetText() const { return _text->text; }

        void SetTextColor(const Color& color);
        const Color& GetTextColor() const { return _text->color; }

        void SetTextOutlineColor(const Color& outlineColor);
        const Color& GetTextOutlineColor() const { return _text->outlineColor; }

        void SetTextOutlineWidth(f32 outlineWidth);
        const f32 GetTextOutlineWidth() const { return _text->outlineWidth; }

        void SetTextFont(const std::string& fontPath, f32 fontSize);

        static InputField* CreateInputField();
    };
}