#pragma once
#include <NovusTypes.h>
#include <angelscript.h>
#include "../../../Scripting/ScriptEngine.h"

namespace UIComponent
{
    struct InputField
    {
    public:
        InputField() { }

        size_t writeHeadIndex = 0;
        asIScriptFunction* onSubmitCallback = nullptr;
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
        void OnSubmit()
        {
            if (!onSubmitCallback)
                return;

            _OnEvent(onSubmitCallback);
        }
    };
}