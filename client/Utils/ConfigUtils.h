#pragma once
#include <entity/fwd.hpp>
#include <filesystem>
namespace fs = std::filesystem;

enum class ConfigSaveType
{
    ALL,
    CVAR,
    UI
};

struct ConfigSingleton;
namespace ConfigUtils
{
    const fs::path configFolderPath = fs::path("Data/configs").make_preferred();
    const fs::path cvarConfigPath = (configFolderPath / "CVarConfig.json").make_preferred();
    const fs::path uiConfigPath = (configFolderPath / "UIConfig.json").make_preferred();

    bool SaveConfig(ConfigSaveType type);
}