#pragma once
#include <angelscript.h>
#include "../../../Scripting/ScriptEngine.h"

namespace UIComponent
{
    struct Checkbox
    {
    public:
        Checkbox() { }

        bool checked = false;

        asIScriptFunction* onChecked = nullptr;
        asIScriptFunction* onUnchecked = nullptr;
        void* asObject = nullptr;

        // Usually Components do not store logic, however this is an exception
    private:
        void _OnEvent(asIScriptFunction* callback)
        {
            asIScriptContext* context = ScriptEngine::GetScriptContext();
            {
                context->Prepare(callback);
                {
                    context->SetArgObject(0, asObject);
                }
                context->Execute();
            }
        }
    public:
        void OnChecked()
        {
            if (!onChecked)
                return;

            _OnEvent(onChecked);
        }
        void OnUnchecked()
        {
            if (!onUnchecked)
                return;

            _OnEvent(onUnchecked);
        }
    };
}