#include "RenderModelSystem.h"
#include <entt.hpp>
#include <tracy/Tracy.hpp>
#include <Renderer/Renderer.h>
#include "../../../Utils/ServiceLocator.h"
#include "../../../Rendering/ClientRenderer.h"
#include "../../Components/Transform.h"
#include "../../Components/Rendering/Model.h"
#include "../../Components/Rendering/VisibleModel.h"

void RenderModelSystem::Update(entt::registry& registry, ClientRenderer* clientRenderer)
{
    Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

    // Get all layers in which our models need to be rendered in and call .RegisterModel on the layers
    Renderer::RenderLayer& mainLayer = renderer->GetRenderLayer("MainLayer"_h);
    Renderer::RenderLayer& depthLayer = renderer->GetRenderLayer("DepthPrepass"_h);

    auto modelView = registry.view<Transform, Model, VisibleModel>();
    modelView.each([&](const auto, Transform& transform, Model& model, VisibleModel&)
        {
            if (transform.isDirty)
            {
                model.instanceData.modelMatrix = transform.GetMatrix();

                // We should be using th 
                model.instanceData.Apply(clientRenderer->GetFrameIndex());
                transform.isDirty = false;
            }

            // This registers the model to be rendered THIS frame.
            mainLayer.RegisterModel(model.modelId, &model.instanceData);
            depthLayer.RegisterModel(model.modelId, &model.instanceData);
        });
}