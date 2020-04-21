#include "ScriptHandler.h"

#include <Utils/DebugHandler.h>
#include <Utils/Timer.h>

// Angelscript Addons
#include "Addons/scriptarray/scriptarray.h"
#include "Addons/scriptbuilder/scriptbuilder.h"
#include "Addons/scriptstdstring/scriptstdstring.h"

#include "ScriptEngine.h"
#include "Classes/Player.h"
#include "../Rendering/UIElementRegistry.h"

// NovusCore functions

// NovusCore hooks

namespace fs = std::filesystem;
std::string ScriptHandler::_scriptFolder = "";

void ScriptHandler::ReloadScripts()
{
    UIElementRegistry::Instance()->Clear();

    NC_LOG_MESSAGE("Reloading scripts...");

    if (_scriptFolder != "")
    {
        LoadScriptDirectory(_scriptFolder);
    }
}

void ScriptHandler::LoadScriptDirectory(std::string& scriptFolder)
{
    _scriptFolder = scriptFolder; 
    fs::path absolutePath = fs::absolute(scriptFolder);
    if (!fs::exists(absolutePath))
    {
        fs::create_directory(absolutePath);
    }

    Timer timer;
    size_t count = 0;
    for (auto& scriptPath : fs::recursive_directory_iterator(absolutePath))
    {
        if (scriptPath.is_directory())
            continue;

        if (LoadScript(scriptPath.path()))
        {
            count++;
        }
    }
    f32 msTimeTaken = timer.GetLifeTime() * 1000;
    NC_LOG_SUCCESS("Loaded %u scripts in %.2f ms", count, msTimeTaken);
}

bool ScriptHandler::LoadScript(fs::path scriptPath)
{
    asIScriptEngine* scriptEngine = ScriptEngine::GetScriptEngine();
    std::string moduleName = scriptPath.filename().string();

    CScriptBuilder builder;
    int r = builder.StartNewModule(scriptEngine, moduleName.c_str());
    if (r < 0)
    {
        // If the code fails here it is usually because there
        // is no more memory to allocate the module
        NC_LOG_ERROR("[Script]: Unrecoverable error while starting a new module.");
        return false;
    }
    r = builder.AddSectionFromFile(scriptPath.string().c_str());
    if (r < 0)
    {
        // The builder wasn't able to load the file. Maybe the file
        // has been removed, or the wrong name was given, or some
        // preprocessing commands are incorrectly written.
        NC_LOG_ERROR("[Script]: Please correct the errors in the script and try again.\n");
        return false;
    }
    r = builder.BuildModule();
    if (r < 0)
    {
        // An error occurred. Instruct the script writer to fix the
        // compilation errors that were listed in the output stream.
        NC_LOG_ERROR("[Script]: Please correct the errors in the script and try again.\n");
        return false;
    }

    asIScriptModule* mod = scriptEngine->GetModule(moduleName.c_str());
    asIScriptFunction* func = mod->GetFunctionByDecl("void main()");
    if (func == 0)
    {
        // The function couldn't be found. Instruct the script writer
        // to include the expected function in the script.
        NC_LOG_ERROR("[Script]: The script must have the function 'void main()'. Please add it and try again.\n");
        return false;
    }

    // Create our context, prepare it, and then execute
    asIScriptContext* ctx = scriptEngine->CreateContext();
    ctx->Prepare(func);
    r = ctx->Execute();
    if (r != asEXECUTION_FINISHED)
    {
        // The execution didn't complete as expected. Determine what happened.
        if (r == asEXECUTION_EXCEPTION)
        {
            // An exception occurred, let the script writer know what happened so it can be corrected.
            NC_LOG_ERROR("[Script]: An exception '%s' occurred. Please correct the code and try again.\n", ctx->GetExceptionString());
            ctx->Release();
            return false;
        }
    }

    ctx->Release();
    return true;
}