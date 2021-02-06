#include "Player.h"
#include "../ScriptEngine.h"

void Player::Print()
{
    DebugHandler::Print("Player (%f, %f, %f)", x, y, z);
}

void Player::RegisterType()
{
    i32 r = ScriptEngine::RegisterScriptClass("Player", sizeof(Player), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Player>());
    assert(r >= 0);
    {
        r = ScriptEngine::RegisterScriptClassFunction("void Print()", asMETHOD(Player, Print)); assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassProperty("float x", asOFFSET(Player, x)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassProperty("float y", asOFFSET(Player, y)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassProperty("float z", asOFFSET(Player, z)); assert(r >= 0);
    }
}