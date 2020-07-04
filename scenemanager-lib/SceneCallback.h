#include <NovusTypes.h>

typedef void SceneLoadedFunc(u32 sceneNameHash);
struct SceneCallback
{
    SceneCallback(u32 inCallbackNameHashed, std::function<SceneLoadedFunc> inCallback)
    {
        callbackNameHashed = inCallbackNameHashed;
        callback = inCallback;
    }
    
    u32 callbackNameHashed;
    std::function<SceneLoadedFunc> callback;
};