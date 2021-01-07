#include "TextStylesheet.h"
#include "../../Scripting/ScriptEngine.h"

static void Construct(void* memory)
{
    new(memory) UI::TextStylesheet();
}

static void Construct_FONT(std::string fontPath, f32 fontSize, UI::TextStylesheet* out)
{
    new (out) UI::TextStylesheet();
    out->overrideMask |= UI::TextStylesheet::OverrideMaskProperties::FONT_PATH | UI::TextStylesheet::OverrideMaskProperties::FONT_SIZE;
    out->fontPath = fontPath;
    out->fontSize = fontSize;
}

void UI::TextStylesheet::RegisterType()
{
    // TextStylesheet
    u32 flags = asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS | asOBJ_APP_CLASS_CONSTRUCTOR | asOBJ_APP_CLASS_ASSIGNMENT | asOBJ_APP_CLASS_COPY_CONSTRUCTOR;
    i32 r = ScriptEngine::RegisterScriptClass("TextStylesheet", sizeof(UI::TextStylesheet), flags);
    assert(r >= 0);
    {
        r = ScriptEngine::RegisterScriptClassFunction("void SetFontPath(string font)", asMETHOD(UI::TextStylesheet, SetFontPath)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFontSize(float size)", asMETHOD(UI::TextStylesheet, SetFontSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetLineHeightMultiplier(float lineHeightMultiplier)", asMETHOD(UI::TextStylesheet, SetLineHeightMultiplier)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(UI::TextStylesheet, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(UI::TextStylesheet, SetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float outlineWidth)", asMETHOD(UI::TextStylesheet, SetOutlineWidth)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassFunction("void SetHorizontalAlignment(uint8 alignment)", asMETHOD(UI::TextStylesheet, SetHorizontalAlignment)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetVerticalAlignment(uint8 alignment)", asMETHOD(UI::TextStylesheet, SetVerticalAlignment)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetMultiline(bool multiline)", asMETHOD(UI::TextStylesheet, SetMultiline)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassConstructor("void f()", asFUNCTION(Construct)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassConstructor("void f(string fontPath, float fontSize)", asFUNCTION(Construct_FONT)); assert(r >= 0);
    }
}
