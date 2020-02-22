#include "Player.h"
#include "../ScriptEngine.h"

const char* Player::_objectName = "Player";
void Player::Print()
{
    NC_LOG_MESSAGE("Player (%f, %f, %f)", x, y, z);
}

void Player::RegisterType()
{
    i32 r = ScriptEngine::RegisterScriptClass(_objectName, sizeof(Player), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Player>());
    assert(r >= 0);
    {
        r = ScriptEngine::RegisterScriptClassFunction("void Print()", asMETHOD(Player, Print));
        assert(r >= 0);

        r = ScriptEngine::RegisterScriptClassProperty("float x", asOFFSET(Player, x));
        assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassProperty("float y", asOFFSET(Player, y));
        assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassProperty("float z", asOFFSET(Player, z));
        assert(r >= 0);
    }
}