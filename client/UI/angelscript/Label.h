#pragma once
#include <NovusTypes.h>
#include "../UITypes.h"
#include "BaseElement.h"

namespace UIScripting
{
    class Label : public BaseElement
    {
    public:
        Label();

        static void RegisterType();

        //Text Functions
        const std::string GetText() const;
        void SetText(const std::string& newText);

        void SetStylesheet(const UI::TextStylesheet& textStylesheet);
         
        static Label* CreateLabel();
    };
}