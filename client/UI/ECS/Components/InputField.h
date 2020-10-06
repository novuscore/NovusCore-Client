#pragma once
#include <NovusTypes.h>

class asIScriptFunction;

namespace UIComponent
{
    struct InputField
    {
    public:
        InputField() { }

        size_t writeHeadIndex = 0;
        
        asIScriptFunction* onSubmitCallback = nullptr;
    };
}