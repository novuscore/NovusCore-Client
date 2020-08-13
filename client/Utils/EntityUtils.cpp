#include "EntityUtils.h"
#include <entt.hpp>
#include <Renderer/Renderer.h>
#include "../Utils/ServiceLocator.h"
#include "../ECS/Components/Rendering/Model.h"
#include "../ECS/Components/Rendering/VisibleModel.h"

Model& EntityUtils::CreateModelComponent(entt::registry& registry, entt::entity& entity, std::string modelPath)
{
    Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

    Renderer::ModelDesc desc;
    desc.path = modelPath;

    Model& model = registry.emplace<Model>(entity);
    registry.emplace<VisibleModel>(entity);

    model.modelId = renderer->LoadModel(desc);
    //model.instanceData.Init(renderer);

    return model;
}
