#pragma once
#include <NovusTypes.h>
#include <Utils/StringUtils.h>
#include <robin_hood.h>

class Loader
{
public:
    Loader(StringUtils::StringHash hash, u32 priority);

    u32 GetHash() { return _hash; }
    u32 GetPriority() { return _priority; }

    virtual bool Init() = 0;

private:
    u32 _hash;
    u32 _priority;
};

class LoaderSystem
{
public:
    static LoaderSystem* Get();
    void Init();

    std::vector<Loader*>& GetLoaders() { return _loaders; }
    void AddLoader(Loader* loader);

private:
    bool _isInitialized = false;

    robin_hood::unordered_map<u32, Loader*> _hashToLoader;
    std::vector<Loader*> _loaders;
};