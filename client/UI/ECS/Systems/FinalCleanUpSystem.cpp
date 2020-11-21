#include "FinalCleanUpSystem.h"
#include <entity/registry.hpp>

#include "../Components/Dirty.h"
#include "../Components/BoundsDirty.h"
#include "../Components/SortKeyDirty.h"


namespace UISystem
{
    void FinalCleanUpSystem::Update(entt::registry& registry)
    {
        registry.clear<UIComponent::Dirty>();
        registry.clear<UIComponent::BoundsDirty>();
        registry.clear<UIComponent::SortKeyDirty>();
    }
}