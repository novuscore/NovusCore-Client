#pragma once
#include <angelscript.h>

namespace ASSceneManagerUtils
{
    void RegisterNamespace();

    void OnSceneLoaded(u32 sceneLoaded);
    void RegisterSceneLoadedCallback(std::string sceneName, std::string callbackName, asIScriptFunction* callback);
    void UnregisterSceneLoadedCallback(std::string sceneName, std::string callbackName);
};