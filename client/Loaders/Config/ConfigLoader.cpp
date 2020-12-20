#include "../LoaderSystem.h"
#include "../../Utils/ServiceLocator.h"
#include "../../Utils/ConfigUtils.h"
#include "../../ECS/Components/Singletons/ConfigSingleton.h"

#include <CVar/CVarSystemPrivate.h>

class ConfigLoader : Loader
{
public:
    ConfigLoader() : Loader("ConfigLoader", 1000) { }

    bool Init()
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        ConfigSingleton& configSingleton = registry->set<ConfigSingleton>();

        if (!fs::exists(ConfigUtils::configFolderPath))
            fs::create_directory(ConfigUtils::configFolderPath);

        bool loadingFailed = false;

        JsonConfig& cvarConfig = configSingleton.cvarJsonConfig;
        {
            json defaultConfig;

            // Default CVar Config
            {
                defaultConfig["integer"] = json::object();
                defaultConfig["double"] = json::object();
                defaultConfig["string"] = json::object();
                defaultConfig["vector4"] = json::object();
                defaultConfig["ivector4"] = json::object();
            }

            bool didLoadOrCreate = cvarConfig.LoadOrCreate(ConfigUtils::cvarConfigPath, defaultConfig);
            if (didLoadOrCreate)
            {
                CVarSystem* cvarSystem = CVarSystem::Get();

                json& config = cvarConfig.GetConfig();
                CVarSystemImpl::Get()->LoadCVarsFromJson(config);
                CVarSystemImpl::Get()->LoadCVarsIntoJson(config);
            }

            loadingFailed |= !didLoadOrCreate;
        }

        JsonConfig& uiConfig = configSingleton.uiJsonConfig;
        {
            json defaultConfig;

            // Default CVar Config
            {
                defaultConfig["defaultMap"] = "None";
            }

            bool didLoadOrCreate = uiConfig.LoadOrCreate(ConfigUtils::uiConfigPath, defaultConfig);
            if (didLoadOrCreate)
            {
                json& config = uiConfig.GetConfig();
                configSingleton.uiConfig = config;
            }

            loadingFailed |= !didLoadOrCreate;
        }

        return !loadingFailed;
    }
};

ConfigLoader configLoader;