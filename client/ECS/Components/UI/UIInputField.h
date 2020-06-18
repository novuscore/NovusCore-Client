#pragma once
#include <NovusTypes.h>
#include <angelscript.h>
#include "../../../Scripting/ScriptEngine.h"

// We need to define structs for event data, so we can pass data into callbacks for angelscript
struct UIInputField
{
public:
    UIInputField() : writeHeadIndex() ,onSubmitCallback(nullptr), asObject(nullptr) { }

    u32 writeHeadIndex;
    asIScriptFunction* onSubmitCallback;
    void* asObject;

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