#pragma once
#include <NovusTypes.h>
#include "../Stylesheets/TextStylesheet.h"
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

        void SetStylesheet(const UI::TextStylesheet& styleSheet);

        static InputField* CreateInputField();
    };
}