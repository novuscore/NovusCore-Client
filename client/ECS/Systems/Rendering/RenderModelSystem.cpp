#include "RenderModelSystem.h"
#include <Renderer/Renderer.h>
#include "../../../Utils/ServiceLocator.h"
#include "../../Components/Transform.h"
#include "../../Components/Rendering/Model.h"

void RenderModelSystem::Update(entt::registry& registry)
{
    /* This should occur when we create the entity

    Renderer::ModelDesc desc;
    desc.path = "Data/models/Cube.novusmodel";

    Renderer::ModelID modelId = renderer->LoadModel(desc);*/

    Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

    // Get all layers in which our models need to be rendered in and call .RegisterModel on the layers
    Renderer::RenderLayer& mainLayer = renderer->GetRenderLayer("MainLayer"_h);
    Renderer::RenderLayer& depthLayer = renderer->GetRenderLayer("DepthPrepass"_h);

    auto modelView = registry.view<Transform, Model>();
    modelView.each([&](const auto, Transform& transform, Model& model)
        {
            if (!model.isVisible)
                return;

            if (transform.isDirty)
            {
                model.instanceData.modelMatrix = transform.GetMatrix();
                transform.isDirty = false;
            }

            // This registers the model to be rendered THIS frame.
            mainLayer.RegisterModel(model.modelId, &model.instanceData);
            depthLayer.RegisterModel(model.modelId, &model.instanceData);
        });
}