#include "RegistryUtils.h"
#include <entity/registry.hpp>
#include <angelscript.h>
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"
#include "../ECS/Components/Singletons/UILockSingleton.h"

namespace UIUtils::Registry
{
    void RegisterNamespace()
    {
        i32 r = ScriptEngine::SetNamespace("UI"); assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptFunction("LockToken@ GetLock(uint8 state)", asFUNCTION(GetLock)); assert(r >= 0);
        }
        r = ScriptEngine::ResetNamespace();
    }

    UIScripting::LockToken* GetLock(UIScripting::LockState state)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        auto lockSingleton = &registry->ctx<UISingleton::UILockSingleton>();

        return new UIScripting::LockToken(lockSingleton->mutex, state);
    }
}
