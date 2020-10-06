#pragma once
#include <NovusTypes.h>
#include <angelscript.h>
#include "../../Scripting/ScriptEngine.h"

namespace UIUtils
{
    inline static void ExecuteEvent(void* scriptingObject, asIScriptFunction* scriptFunction)
    {
        if (!scriptingObject || !scriptFunction)
            return;

        asIScriptContext* context = ScriptEngine::GetScriptContext();
        {
            context->Prepare(scriptFunction);
            {
                context->SetArgObject(0, scriptingObject);
            }
            context->Execute();
        }
    }
};