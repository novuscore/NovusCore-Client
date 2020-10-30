#include "UIUtils.h"

#include <angelscript.h>
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include <entity/registry.hpp>
#include "../ECS/Components/Singletons/UILockSingleton.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"

namespace UIUtils
{
    void RegisterNamespace()
    {
        i32 r = ScriptEngine::SetNamespace("UI"); assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptFunction("LockToken@ GetLock(uint8 state)", asFUNCTION(GetLock)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptFunction("BaseElement@ GetElement(Entity entityId)", asFUNCTION(GetElement)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptFunction("vec2 GetResolution()", asFUNCTION(GetResolution)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptFunction("float GetWidth()", asFUNCTION(GetWidth)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptFunction("float GetHeight()", asFUNCTION(GetHeight)); assert(r >= 0);
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

    vec2 GetResolution()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        const auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();

        return dataSingleton->UIRESOLUTION;
    }
    f32 GetWidth()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        const auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();

        return dataSingleton->UIRESOLUTION.x;
    }
    f32 GetHeight()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        const auto dataSingleton = &registry->ctx<UISingleton::UIDataSingleton>();

        return dataSingleton->UIRESOLUTION.y;
    }
}
