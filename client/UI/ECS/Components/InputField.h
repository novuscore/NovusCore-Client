#pragma once
#include <NovusTypes.h>
#include <angelscript.h>

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