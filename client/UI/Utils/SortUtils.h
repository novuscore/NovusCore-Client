#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
#include "../UITypes.h"

namespace UIUtils::Sort
{
    /*
    *   Recursively updates depths of children changing it by modifier.
    *   registry: Pointer to UI Registry.
    *   transform: Transform from which to start update.
    *   modifer: amount to modify depth by.
    */
    void UpdateChildDepths(entt::registry* registry, entt::entity parent);
};