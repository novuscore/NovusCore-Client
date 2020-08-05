#include <NovusTypes.h>
#include <robin_hood.h>
#include <vector>
#include "SceneCallback.h"

class SceneManager
{
public:
    SceneManager() : _sceneNameHashes(), _currentSceneNameHashed(0) { }

    void SetAvailableScenes(std::vector<u32> sceneNameHashes)
    {
        _sceneNameHashes.clear();
        _sceneNameHashes.reserve(sceneNameHashes.size());
        _sceneNameHashes.assign(sceneNameHashes.begin(), sceneNameHashes.end());

        _sceneAnyLoadedCallback.clear();
        _sceneAnyLoadedCallback.reserve(sceneNameHashes.size());

        _sceneLoadedCallback.clear();
        _sceneLoadedCallback.reserve(sceneNameHashes.size());
        for (u32 sceneNameHashed : _sceneNameHashes)
        {
            std::vector<SceneCallback> sceneCallbackVector;
            sceneCallbackVector.reserve(8);

            _sceneLoadedCallback[sceneNameHashed] = sceneCallbackVector;
        }
    }

    inline bool SceneExists(u32 sceneNameHashed)
    {
        auto itr = std::find(_sceneNameHashes.begin(), _sceneNameHashes.end(), sceneNameHashed);
        return itr != _sceneNameHashes.end();
    }
    bool LoadScene(u32 sceneNameHashed);
    u32 GetScene() { return _currentSceneNameHashed; }

    bool RegisterSceneLoadedCallback(u32 sceneNameHashed, SceneCallback inSceneCallback);
    bool RegisterSceneLoadedCallback(SceneCallback inSceneCallback);
    bool UnregisterSceneLoadedCallback(u32 sceneNameHashed, u32 callbackNameHashed);
    bool UnregisterSceneLoadedCallback(u32 callbackNameHashed);

private:
    std::vector<u32> _sceneNameHashes;
    u32 _currentSceneNameHashed;

    std::vector<SceneCallback> _sceneAnyLoadedCallback;
    robin_hood::unordered_map<u32, std::vector<SceneCallback>> _sceneLoadedCallback;
};