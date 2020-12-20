#pragma once
#include <string>
#include <filesystem>
#include <asio.hpp>
#include <entity/fwd.hpp>
#include "angelscript.h"

class ScriptHandler
{
public:
    static void Init(entt::registry& registry);
    static void LoadScriptDirectory(std::string& path);
    static void ReloadScripts();

private:
    static bool LoadScript(std::filesystem::path path);

    ScriptHandler();

private:
    static std::string _scriptFolder;
};