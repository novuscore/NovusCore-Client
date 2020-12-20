#include "LoaderSystem.h"

Loader::Loader(StringUtils::StringHash hash, u32 priority)
{
    _priority = priority;

    LoaderSystem* loaderSystem = LoaderSystem::Get();
    loaderSystem->AddLoader(this);
}

LoaderSystem* LoaderSystem::Get()
{
    static LoaderSystem loaderSystem{};
    return &loaderSystem;
}

void LoaderSystem::AddLoader(Loader* loader)
{
    _loaders.push_back(loader);
    _hashToLoader[loader->GetHash()] = loader;
}

void LoaderSystem::Init()
{
    assert(_isInitialized == false);
    _isInitialized = true;

    // Sort Loaders by Priority
    std::sort(_loaders.begin(), _loaders.end(), [](Loader* lhs, Loader* rhs)
    {
        return lhs->GetPriority() > rhs->GetPriority();
    });
}
