#include "UpdateImageModelSystem.h"
#include <entity/registry.hpp>
#include <tracy/Tracy.hpp>
#include <Renderer/Buffer.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include "../../UITypes.h"
#include "../../../Utils/ServiceLocator.h"
#include "../Components/Transform.h"
#include "../Components/Dirty.h"
#include "../Components/NotCulled.h"
#include "../Components/Image.h"

#include "../../Utils/TransformUtils.h"
#include "../../Utils/RenderModelUtils.h"

namespace UISystem
{
    void UpdateImageModelSystem::Update(entt::registry& registry)
    {        
        Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

        auto imageView = registry.view<UIComponent::Transform, UIComponent::Image, UIComponent::Dirty, UIComponent::NotCulled>();
        imageView.each([&](UIComponent::Transform& transform, UIComponent::Image& image)
        {
            ZoneScopedNC("UpdateRenderingSystem::Update::ImageView", tracy::Color::RoyalBlue);
            if (image.style.texture.length() == 0)
                return;

            // Create constant buffer if necessary
            auto constantBuffer = image.constantBuffer;
            if (constantBuffer == nullptr)
            {
                constantBuffer = new Renderer::Buffer<UIComponent::Image::ImageConstantBuffer>(renderer, "UpdateElementSystemConstantBuffer", Renderer::UNIFORM_BUFFER, Renderer::BufferCPUAccess::WriteOnly);
                image.constantBuffer = constantBuffer;
            }
            constantBuffer->resource.color = image.style.color;
            constantBuffer->resource.borderSize = image.style.borderSize;
            constantBuffer->resource.borderInset = image.style.borderInset;
            constantBuffer->resource.slicingOffset = image.style.slicingOffset;
            constantBuffer->resource.size = transform.size;
            constantBuffer->ApplyAll();

            // Transform Updates.
            const vec2 pos = UIUtils::Transform::GetMinBounds(transform);
            const vec2 size = transform.size;
            const UI::FBox& texCoordScaler = image.texCoordScaler;
            const UI::FBox& texCoords = image.style.texCoord;
            const UI::FBox scaledTexCoords = { texCoordScaler.top * texCoords.top, texCoordScaler.right * texCoords.right, texCoordScaler.bottom * texCoords.bottom, texCoordScaler.left * texCoords.left };

            UI::UIVertex vertices[4] = {};
            UIUtils::RenderModel::CreatePanelVertices(&registry, pos, size, scaledTexCoords, &vertices[0]);

            constexpr u32 bufferSize = sizeof(UI::UIVertex) * 4; // 4 vertices per image

            if (image.vertexBufferID == Renderer::BufferID::Invalid())
            {
                Renderer::BufferDesc desc { "ImageVertices", Renderer::BufferUsage::UNIFORM_BUFFER, Renderer::BufferCPUAccess::WriteOnly };
                desc.size = bufferSize;

                image.vertexBufferID = renderer->CreateBuffer(desc);
            }

            void* dst = renderer->MapBuffer(image.vertexBufferID);
            memcpy(dst, vertices, bufferSize);
            renderer->UnmapBuffer(image.vertexBufferID);
        });
    }
}