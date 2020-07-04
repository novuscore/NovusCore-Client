#include "SceneManager.h"

bool SceneManager::LoadScene(u32 sceneNameHashed)
{
    if (!SceneExists(sceneNameHashed))
        return false;

    _currentSceneNameHashed = sceneNameHashed;

    for (auto& sceneCallback : _sceneAnyLoadedCallback)
    {
        sceneCallback.callback(sceneNameHashed);
    }

    for (auto& sceneCallback : _sceneLoadedCallback[sceneNameHashed])
    {
        sceneCallback.callback(sceneNameHashed);
    }

    return true;
}

bool SceneManager::RegisterSceneLoadedCallback(u32 sceneNameHashed, SceneCallback inSceneCallback)
{
    if (!SceneExists(sceneNameHashed))
        return false;

    for (auto& sceneCallback : _sceneLoadedCallback[sceneNameHashed])
    {
        if (inSceneCallback.callbackNameHashed == sceneCallback.callbackNameHashed)
            return false;
    }
    
    _sceneLoadedCallback[sceneNameHashed].push_back(inSceneCallback);
    return true;
}
bool SceneManager::RegisterSceneLoadedCallback(SceneCallback inSceneCallback)
{
    for (auto& sceneCallback : _sceneAnyLoadedCallback)
    {
        if (inSceneCallback.callbackNameHashed == sceneCallback.callbackNameHashed)
            return false;
    }
    
    _sceneAnyLoadedCallback.push_back(inSceneCallback);
    return true;
}

bool SceneManager::UnregisterSceneLoadedCallback(u32 sceneNameHashed, u32 callbackNameHashed)
{
    if (!SceneExists(sceneNameHashed))
        return false;

    auto itr = std::find_if(_sceneLoadedCallback[sceneNameHashed].begin(), _sceneLoadedCallback[sceneNameHashed].end(), [callbackNameHashed] (const SceneCallback& sceneCallback) -> bool { return callbackNameHashed == sceneCallback.callbackNameHashed; });
    if (itr == _sceneLoadedCallback[sceneNameHashed].end())
        return false;

    _sceneLoadedCallback[sceneNameHashed].erase(itr);
    return true;
}
bool SceneManager::UnregisterSceneLoadedCallback(u32 callbackNameHashed)
{
    auto itr = std::find_if(_sceneAnyLoadedCallback.begin(), _sceneAnyLoadedCallback.end(), [callbackNameHashed](const SceneCallback& sceneCallback) -> bool { return callbackNameHashed == sceneCallback.callbackNameHashed; });
    if (itr == _sceneAnyLoadedCallback.end())
        return false;

    _sceneAnyLoadedCallback.erase(itr);
    return true;
}

inline bool SceneManager::SceneExists(u32 sceneNameHashed)
{
    auto itr = std::find(_sceneNameHashes.begin(), _sceneNameHashes.end(), sceneNameHashed);
    return itr != _sceneNameHashes.end();
}
