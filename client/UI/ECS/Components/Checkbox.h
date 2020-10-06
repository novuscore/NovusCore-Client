#pragma once

class asIScriptFunction;

namespace UIComponent
{
    struct Checkbox
    {
    public:
        Checkbox() { }

        bool checked = true;

        asIScriptFunction* onChecked = nullptr;
        asIScriptFunction* onUnchecked = nullptr;
    };
}