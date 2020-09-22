#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>

namespace UIUtils::Sort
{
    /*
    *   Recursively updates depths of children changing it by modifier.
    *   registry: Pointer to UI Registry.
    *   transform: Transform from which to start update.
    *   modifer: amount to modify depth by.
    *   THREAD-SAFE
    */
    void UpdateChildDepths(entt::registry* registry, entt::entity parent, i16 modifier);
};