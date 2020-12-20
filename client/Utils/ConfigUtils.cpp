#include "ConfigUtils.h"
#include "../Utils/ServiceLocator.h"
#include "../ECS/Components/Singletons/ConfigSingleton.h"

#include <NovusTypes.h>
#include <entt.hpp>
#include <CVar/CVarSystemPrivate.h>

namespace ConfigUtils
{
    bool SaveConfig(ConfigSaveType type)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        ConfigSingleton& configSingleton = registry->ctx<ConfigSingleton>();

        bool savingFailed = false;
        bool savingAll = type == ConfigSaveType::ALL;

        if (savingAll || type == ConfigSaveType::CVAR)
        {
            json& config = configSingleton.cvarJsonConfig.GetConfig();
            CVarSystemImpl::Get()->LoadCVarsIntoJson(config);
            savingFailed |= !configSingleton.cvarJsonConfig.Save(cvarConfigPath);
        }

        if (savingAll || type == ConfigSaveType::UI)
        {
            json& config = configSingleton.uiJsonConfig.GetConfig();
            config = configSingleton.uiConfig;

            savingFailed |= !configSingleton.uiJsonConfig.Save(uiConfigPath);
        }

        return !savingFailed;
    }
}
