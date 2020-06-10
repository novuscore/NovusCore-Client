#pragma once
#include <string>
#include <entity/fwd.hpp>

struct Model;
namespace EntityUtils
{
    // This function modifies the registry thus it should only be called from the main thread (entt:registry is not thread-safe)
    Model& CreateModelComponent(entt::registry& registry, entt::entity& entity, std::string modelPath);
}