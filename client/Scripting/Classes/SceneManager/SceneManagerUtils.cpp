#include "SceneManagerUtils.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../scenemanager-lib/SceneManager.h"

#include "../../../ECS/Components/Singletons/SceneManagerSingleton.h"

namespace ASSceneManagerUtils
{
    void RegisterNamespace()
    {
        i32 r = ScriptEngine::SetNamespace("SceneManager");

        r = ScriptEngine::RegisterScriptFunctionDef("void SceneLoadedCallback(uint sceneHash)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunction("void RegisterSceneLoadedCallback(string sceneName, string callBackName, SceneLoadedCallback@ cb)", asFUNCTION(RegisterSceneLoadedCallback)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunction("void UnRegisterSceneLoadedCallback(string sceneName, string callBackName)", asFUNCTION(UnregisterSceneLoadedCallback)); assert(r >= 0);

        SceneManager* sceneManager = ServiceLocator::GetSceneManager();
        sceneManager->RegisterSceneLoadedCallback(SceneCallback("UIRenderer Callback"_h, std::bind(OnSceneLoaded, std::placeholders::_1)));

        r = ScriptEngine::ResetNamespace();
        assert(r >= 0);
    }

    void OnSceneLoaded(u32 sceneLoaded)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        SceneManagerSingleton& scriptSceneSingleton = registry->ctx<SceneManagerSingleton>();

        for (auto& sceneCallback : scriptSceneSingleton.sceneAnyLoadedCallback)
        {
            asIScriptContext* context = ScriptEngine::GetScriptContext();
            {
                context->Prepare(sceneCallback.callback);
                {
                    context->SetArgDWord(0, sceneLoaded);
                }
                context->Execute();
            }
        }

        for (auto& sceneCallback : scriptSceneSingleton.sceneLoadedCallback[sceneLoaded])
        {
            asIScriptContext* context = ScriptEngine::GetScriptContext();
            {
                context->Prepare(sceneCallback.callback);
                {
                    context->SetArgDWord(0, sceneLoaded);
                }
                context->Execute();
            }
        }
    }
    
    void RegisterSceneLoadedCallback(std::string sceneName, std::string callbackName, asIScriptFunction* callback)
    {
        u32 sceneHash = StringUtils::fnv1a_32(sceneName.c_str(), sceneName.size());
        u32 callbackNameHash = StringUtils::fnv1a_32(callbackName.c_str(), callbackName.size());

        if (!ServiceLocator::GetSceneManager()->SceneExists(sceneHash))
            return;

        entt::registry* registry = ServiceLocator::GetGameRegistry();
        SceneManagerSingleton& scriptSceneSingleton = registry->ctx<SceneManagerSingleton>();

        for (auto& sceneCallback : scriptSceneSingleton.sceneLoadedCallback[sceneHash])
        {
            if (callbackNameHash == sceneCallback.callbackNameHashed)
                return;
        }

        scriptSceneSingleton.sceneLoadedCallback[sceneHash].push_back(asSceneCallback(callbackNameHash, callback));
    }
    
    void UnregisterSceneLoadedCallback(std::string sceneName, std::string callbackName)
    {
        u32 sceneHash = StringUtils::fnv1a_32(sceneName.c_str(), sceneName.size());
        u32 callbackNameHash = StringUtils::fnv1a_32(callbackName.c_str(), callbackName.size());

        if (!ServiceLocator::GetSceneManager()->SceneExists(sceneHash))
            return;

        entt::registry* registry = ServiceLocator::GetUIRegistry();
        SceneManagerSingleton& scriptSceneSingleton = registry->ctx<SceneManagerSingleton>();

        auto itr = std::find_if(scriptSceneSingleton.sceneLoadedCallback[sceneHash].begin(), scriptSceneSingleton.sceneLoadedCallback[sceneHash].end(), [callbackNameHash](const asSceneCallback& sceneCallback) -> bool { return callbackNameHash == sceneCallback.callbackNameHashed; });
        if (itr == scriptSceneSingleton.sceneLoadedCallback[sceneHash].end())
            return;

        scriptSceneSingleton.sceneLoadedCallback[sceneHash].erase(itr);
    }
}
