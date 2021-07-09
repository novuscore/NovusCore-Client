#pragma once
#include <NovusTypes.h>
#include "EventElement.h"

namespace UI
{
    struct TextStylesheet;
}

namespace UIScripting
{
    class InputField : public EventElement
    {
    public:
        InputField(const std::string& name, bool collisionEnabled);

        static void RegisterType();

        bool OnKeyInput(i32 key) override;
        bool OnCharInput(char c) override;

        void RemovePreviousCharacter();
        void RemoveNextCharacter();

        void MovePointerLeft();
        void MovePointerRight();

        void SetWriteHeadPosition(size_t position);

        void SetOnSubmitCallback(asIScriptFunction* callback);

        //Label Functions
        const std::string GetText() const;
        void SetText(const std::string& newText, bool updateWriteHead = true);

        void SetStylesheet(const UI::TextStylesheet& styleSheet);

        static InputField* CreateInputField(const std::string& name, bool collisionEnabled = true);
    };
}