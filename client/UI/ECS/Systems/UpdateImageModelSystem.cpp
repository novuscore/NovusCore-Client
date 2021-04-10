#include "UpdateImageModelSystem.h"
#include <entity/registry.hpp>
#include <tracy/Tracy.hpp>
#include <Renderer/Buffer.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include "../../UITypes.h"
#include "../../../Utils/ServiceLocator.h"

#include "../Components/Singletons/UIDataSingleton.h"
#include "../Components/Transform.h"
#include "../Components/Dirty.h"
#include "../Components/Image.h"

#include "../../Utils/TransformUtils.h"

namespace UISystem
{
    inline void CalculateVertices(const vec2& pos, const vec2& size, const UI::FBox& texCoords, UI::UIVertex* vertices)
    {
        const UISingleton::UIDataSingleton& dataSingleton = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>();

        vec2 upperLeftPos = vec2(pos.x, pos.y);
        vec2 upperRightPos = vec2(pos.x + size.x, pos.y);
        vec2 lowerLeftPos = vec2(pos.x, pos.y + size.y);
        vec2 lowerRightPos = vec2(pos.x + size.x, pos.y + size.y);

        // Convert positions to UI render space (0.0 to 1.0).
        upperLeftPos /= dataSingleton.UIRESOLUTION;
        upperRightPos /= dataSingleton.UIRESOLUTION;
        lowerLeftPos /= dataSingleton.UIRESOLUTION;
        lowerRightPos /= dataSingleton.UIRESOLUTION;

        // Flip Y.
        upperLeftPos.y = 1.0f - upperLeftPos.y;
        upperRightPos.y = 1.0f - upperRightPos.y;
        lowerLeftPos.y = 1.0f - lowerLeftPos.y;
        lowerRightPos.y = 1.0f - lowerRightPos.y;

        // UI Vertices. (Pos, UV)
        vertices[0] = { upperLeftPos, vec2(texCoords.left, texCoords.top) };

        vertices[1] = { upperRightPos, vec2(texCoords.right, texCoords.top) };

        vertices[2] = { lowerLeftPos, vec2(texCoords.left, texCoords.bottom) };

        vertices[3] = { lowerRightPos, vec2(texCoords.right, texCoords.bottom) };
    }

    void UpdateImageModelSystem::Update(entt::registry& registry)
    {        
        Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

        auto imageView = registry.view<UIComponent::Transform, UIComponent::Image, UIComponent::Dirty>();
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
            const UI::FBox& texCoords = image.style.texCoord;

            UI::UIVertex vertices[4] = {};
            CalculateVertices(pos, size, texCoords, &vertices[0]);

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