#include "UIUtils.h"

#include <angelscript.h>
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

#include <entity/registry.hpp>
#include "../ECS/Components/Singletons/UIDataSingleton.h"

namespace UIUtils
{
    void RegisterNamespace()
    {
        i32 r = ScriptEngine::SetNamespace("UI"); assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptFunction("BaseElement@ GetElement(Entity entityId)", asFUNCTION(GetElement)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptFunction("vec2 GetResolution()", asFUNCTION(GetResolution)); assert(r >= 0);
        }
        r = ScriptEngine::ResetNamespace();
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
}
