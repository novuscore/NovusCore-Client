#pragma once
#include <NovusTypes.h>
#include <vector>
#include <robin_hood.h>

#include "../../../Scripting/ScriptEngine.h"

struct asSceneCallback
{
    asSceneCallback(u32 inCallbackNameHashed, asIScriptFunction* inCallback)
    {
        callbackNameHashed = inCallbackNameHashed;
        callback = inCallback;
    }

    u32 callbackNameHashed;
    asIScriptFunction* callback;
};

struct ScriptSceneSingleton
{
    ScriptSceneSingleton() {}

    std::vector<asSceneCallback> sceneAnyLoadedCallback;
    robin_hood::unordered_map<u32, std::vector<asSceneCallback>> sceneLoadedCallback;
};