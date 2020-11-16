#pragma once
#include <Utils/JsonConfig.h>

struct UIConfig
{
public:
    const std::string& GetDefaultMap() const { return _defaultMap; }
    void SetDefaultMap(std::string map) { _defaultMap = map; MarkDirty(); }

    void MarkDirty() { _isDirty = true; }
    void ClearDirty() { _isDirty = false; }
    bool IsDirty() { return _isDirty; }

private:
    bool _isDirty = false;
    std::string _defaultMap = "";
};

inline void to_json(json& j, const UIConfig& uiConfig) {
    j = { { "defaultMap", uiConfig.GetDefaultMap() } };
};

inline void from_json(const json& j, UIConfig& uiConfig) {
    uiConfig.SetDefaultMap(j.at("defaultMap").get<std::string>());
}

struct ConfigSingleton
{
    JsonConfig cvarJsonConfig;

    JsonConfig uiJsonConfig;
    UIConfig uiConfig;
};