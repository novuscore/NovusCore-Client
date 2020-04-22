#include "ColorUtil.h"
#include <NovusTypes.h>
#include "Math/Color.h"
#include "../../dep/angelscript/angelscript/as_scriptengine.h"
#include "../../ScriptEngine.h"

static void Construct_RGBA(float r, float g, float b, float a, Color* out)
{
    out->r = r;
    out->g = g;
    out->b = b;
    out->a = a;
}

static void Construct_RGB(float r, float g, float b, Color* out)
{
    out->r = r;
    out->g = g;
    out->b = b;
    out->a = 1;
}

void ColorUtil::RegisterType()
{
    i32 result = ScriptEngine::RegisterScriptClass("Color", sizeof(Color),
        asOBJ_VALUE |
        asOBJ_POD |
        asOBJ_APP_CLASS |
        asOBJ_APP_CLASS_CONSTRUCTOR |
        asOBJ_APP_CLASS_ASSIGNMENT |
        asOBJ_APP_CLASS_COPY_CONSTRUCTOR |
        asOBJ_APP_CLASS_ALLFLOATS);

    assert(result >= 0);
    {
        result = ScriptEngine::RegisterScriptClassProperty("float r", asOFFSET(Color, r)); assert(result >= 0);
        result = ScriptEngine::RegisterScriptClassProperty("float g", asOFFSET(Color, g)); assert(result >= 0);
        result = ScriptEngine::RegisterScriptClassProperty("float b", asOFFSET(Color, b)); assert(result >= 0);
        result = ScriptEngine::RegisterScriptClassProperty("float a", asOFFSET(Color, a)); assert(result >= 0);

        result = ScriptEngine::RegisterScriptClassConstructor("void f(float r, float g, float b, float a)", asFUNCTION(Construct_RGBA)); assert(result >= 0);
        result = ScriptEngine::RegisterScriptClassConstructor("void f(float r, float g, float b)", asFUNCTION(Construct_RGB)); assert(result >= 0);
    }
}
