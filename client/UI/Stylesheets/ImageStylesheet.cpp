#include "ImageStylesheet.h"
#include "../../Scripting/ScriptEngine.h"

static void Construct_IMAGESTYLESHEET(void* memory)
{
    new(memory) UI::ImageStylesheet();
}

void UI::ImageStylesheet::RegisterType()
{
    // ImageStylesheet        
    u32 flags = asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS | asOBJ_APP_CLASS_CONSTRUCTOR | asOBJ_APP_CLASS_ASSIGNMENT | asOBJ_APP_CLASS_COPY_CONSTRUCTOR;
    i32 r = ScriptEngine::RegisterScriptClass("ImageStylesheet", sizeof(UI::ImageStylesheet), flags);
    assert(r >= 0);
    {        
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(UI::ImageStylesheet, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexCoord(FBox texCoord)", asMETHOD(UI::ImageStylesheet, SetTexCoord)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(UI::ImageStylesheet, SetColor)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderTexture(string texture)", asMETHOD(UI::ImageStylesheet, SetBorderTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderSize(Box borderSize)", asMETHOD(UI::ImageStylesheet, SetBorderSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderInset(Box borderInset)", asMETHOD(UI::ImageStylesheet, SetBorderInset)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassConstructor("void f()", asFUNCTION(Construct_IMAGESTYLESHEET)); assert(r >= 0);
    }
}
