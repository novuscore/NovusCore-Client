#include "RegistryUtils.h"
#include <entity/registry.hpp>
#include <angelscript.h>
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"
#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"

namespace UIUtils::Registry
{
    void RegisterNamespace()
    {
        i32 r = ScriptEngine::SetNamespace("UI"); assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptFunction("LockToken@ GetLock(uint8 state)", asFUNCTION(GetLock)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptFunction("BaseElement@ GetElement(Entity entityId)", asFUNCTION(GetElement)); assert(r >= 0);
        }
        r = ScriptEngine::ResetNamespace();
    }

    UIScripting::LockToken* GetLock(UIScripting::LockState state)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto lockSingleton = &registry->ctx<UISingleton::UILockSingleton>();

        return new UIScripting::LockToken(lockSingleton->mutex, state);
    }

    UIScripting::BaseElement* GetElement(entt::entity entityId)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();

        return dataSingleton->entityToElement[entityId];
    }
}
