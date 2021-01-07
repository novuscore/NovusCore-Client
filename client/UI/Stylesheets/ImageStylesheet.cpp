#include "ImageStylesheet.h"
#include "../../Scripting/ScriptEngine.h"

static void Construct(void* memory)
{
    new(memory) UI::ImageStylesheet();
}

static void Construct_TEXTURE(std::string texture, UI::ImageStylesheet* out)
{
    new (out) UI::ImageStylesheet();
    out->SetTexture(texture);
}

void UI::ImageStylesheet::RegisterType()
{
    // ImageStylesheet
    i32 r = ScriptEngine::RegisterScriptClass("ImageStylesheet", sizeof(UI::ImageStylesheet), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<UI::ImageStylesheet>());
    assert(r >= 0);
    {        
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(UI::ImageStylesheet, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexCoord(FBox texCoord)", asMETHOD(UI::ImageStylesheet, SetTexCoord)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(UI::ImageStylesheet, SetColor)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderTexture(string texture)", asMETHOD(UI::ImageStylesheet, SetBorderTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderSize(Box borderSize)", asMETHOD(UI::ImageStylesheet, SetBorderSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetBorderInset(Box borderInset)", asMETHOD(UI::ImageStylesheet, SetBorderInset)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassConstructor("void f()", asFUNCTION(Construct)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassConstructor("void f(string texture)", asFUNCTION(Construct_TEXTURE)); assert(r >= 0);
    }
}
