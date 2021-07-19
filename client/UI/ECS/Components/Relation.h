#pragma once
#include <NovusTypes.h>
#include <vector>
#include <entity/entity.hpp>
#include "../../../UI/UITypes.h"

namespace UIComponent
{
    struct Relation
    {
        Relation()
        {
            children.reserve(8);
        }

        entt::entity parent = entt::null;
        std::vector<entt::entity> children;
    };
}