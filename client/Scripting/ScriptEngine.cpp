#include "ScriptEngine.h"
#include <Utils/DebugHandler.h>

// Types to register
#include "Addons/scriptarray/scriptarray.h"
#include "Addons/scriptstdstring/scriptstdstring.h"
#include "Classes/Math/Math.h"
#include "Classes/Math/ColorUtil.h"
#include "Classes/Player.h"

#include "Classes/UI/asUITransform.h"
#include "Classes/UI/asPanel.h"
#include "Classes/UI/asLabel.h"
#include "Classes/UI/asButton.h"
#include "Classes/UI/asInputfield.h"

thread_local asIScriptEngine* ScriptEngine::_scriptEngine = nullptr;
thread_local asIScriptContext* ScriptEngine::_scriptContext = nullptr;
thread_local std::string ScriptEngine::_scriptCurrentObjectName = "";

void ScriptEngine::Initialize()
{
    if (!_scriptEngine)
    {
        _scriptEngine = asCreateScriptEngine();
        _scriptEngine->SetEngineProperty(asEP_DISALLOW_GLOBAL_VARS, true);
        RegisterFunctions();
    }

    if (!_scriptContext)
    {
        _scriptContext = _scriptEngine->CreateContext();
    }
}

asIScriptEngine* ScriptEngine::GetScriptEngine()
{
    Initialize();
    return _scriptEngine;
}

asIScriptContext* ScriptEngine::GetScriptContext()
{
    Initialize();
    return _scriptContext;
}

i32 ScriptEngine::SetNamespace(std::string name)
{
    return _scriptEngine->SetDefaultNamespace(name.c_str());
}

i32 ScriptEngine::ResetNamespace()
{
    return _scriptEngine->SetDefaultNamespace("");
}

i32 ScriptEngine::RegisterScriptClass(std::string name, i32 byteSize, u32 flags)
{
    _scriptCurrentObjectName = name;
    return _scriptEngine->RegisterObjectType(name.c_str(), byteSize, flags);
}

i32 ScriptEngine::RegisterScriptClassConstructor(std::string declaration, const asSFuncPtr& functionPointer)
{
    return _scriptEngine->RegisterObjectBehaviour(_scriptCurrentObjectName.c_str(), asBEHAVE_CONSTRUCT, declaration.c_str(), functionPointer, asCALL_CDECL_OBJLAST);
}

i32 ScriptEngine::RegisterScriptClassFunction(std::string declaration, const asSFuncPtr& functionPointer, asECallConvTypes callConvType, void* auxiliary, i32 compositeOffset, bool isCompositeIndirect)
{
    return _scriptEngine->RegisterObjectMethod(_scriptCurrentObjectName.c_str(), declaration.c_str(), functionPointer, callConvType, auxiliary, compositeOffset, isCompositeIndirect);
}

i32 ScriptEngine::RegisterScriptClassProperty(std::string declaration, i32 byteOffset, i32 compositeOffset, bool isCompositeIndirect)
{
    return _scriptEngine->RegisterObjectProperty(_scriptCurrentObjectName.c_str(), declaration.c_str(), byteOffset, compositeOffset, isCompositeIndirect);
}

i32 ScriptEngine::RegisterScriptFunction(std::string declaration, const asSFuncPtr& functionPointer, void* auxiliary)
{
    return _scriptEngine->RegisterGlobalFunction(declaration.c_str(), functionPointer, asCALL_CDECL, auxiliary);
}

i32 ScriptEngine::RegisterScriptFunctionDef(std::string declaration)
{
    return _scriptEngine->RegisterFuncdef(declaration.c_str());
}

void ScriptEngine::RegisterFunctions()
{
    // Defaults
    int result;
    result = _scriptEngine->SetMessageCallback(asFUNCTION(ScriptEngine::MessageCallback), 0, asCALL_CDECL);
    assert(result >= 0);
    RegisterScriptArray(_scriptEngine, true);
    RegisterStdString(_scriptEngine);
    RegisterStdStringUtils(_scriptEngine);

    // NovusCore Types
    ASMath::RegisterNamespace();
    ColorUtil::RegisterType();

    Player::RegisterType();
    UI::asUITransform::RegisterType();
    UI::asPanel::RegisterType();
    UI::asLabel::RegisterType();
    UI::asButton::RegisterType();
    UI::asInputfield::RegisterType();

    ScriptEngine::RegisterScriptFunction("void Print(string msg)", asFUNCTION(ScriptEngine::Print));
}

void ScriptEngine::MessageCallback(const asSMessageInfo* msg, void* param)
{
    if (msg->type == asMSGTYPE_ERROR)
    {
        NC_LOG_ERROR("[Script]: %s (%d, %d) : %s\n", msg->section, msg->row, msg->col, msg->message);
    }
    else if (msg->type == asMSGTYPE_WARNING)
    {
        NC_LOG_WARNING("[Script]: %s (%d, %d) : %s\n", msg->section, msg->row, msg->col, msg->message);
    }
    else if (msg->type == asMSGTYPE_INFORMATION)
    {
        NC_LOG_MESSAGE("[Script]: %s (%d, %d) : %s\n", msg->section, msg->row, msg->col, msg->message);
    }
}

void ScriptEngine::Print(std::string& message)
{
    NC_LOG_MESSAGE("[Script]: %s", message.c_str());
}
