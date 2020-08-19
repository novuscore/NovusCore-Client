#include "UpdateElementSystem.h"
#include <entity/registry.hpp>
#include <tracy/Tracy.hpp>
#include "../../render-lib/Renderer/Descriptors/ModelDesc.h"
#include "../../render-lib/Renderer/Buffer.h"

#include "../../../Utils/ServiceLocator.h"
#include "../Components/Transform.h"
#include "../Components/Image.h"
#include "../Components/Text.h"
#include "../Components/Dirty.h"
#include "../Components/BoundsDirty.h"
#include "../Components/Visible.h"
#include "../Components/Collidable.h"
#include "../Components/Singletons/UIDataSingleton.h"
#include "../../Utils/TransformUtils.h"
#include "../../Utils/TextUtils.h"
#include "../../angelscript/BaseElement.h"


namespace UISystem
{
    void CalculateVertices(const vec2& pos, const vec2& size, std::vector<UISystem::UIVertex>& vertices)
    {
        const UISingleton::UIDataSingleton& dataSingleton = ServiceLocator::GetUIRegistry()->ctx<UISingleton::UIDataSingleton>();

        vec2 upperLeftPos = vec2(pos.x, pos.y);
        vec2 upperRightPos = vec2(pos.x + size.x, pos.y);
        vec2 lowerLeftPos = vec2(pos.x, pos.y + size.y);
        vec2 lowerRightPos = vec2(pos.x + size.x, pos.y + size.y);

        // UV space
        // TODO: Do scaling depending on rendertargets actual size instead of assuming 1080p (which is our reference resolution)
        upperLeftPos /= dataSingleton.UIRESOLUTION;
        upperRightPos /= dataSingleton.UIRESOLUTION;
        lowerLeftPos /= dataSingleton.UIRESOLUTION;
        lowerRightPos /= dataSingleton.UIRESOLUTION;

        vertices.reserve(4);

        // UI Vertices
        UISystem::UIVertex& upperLeft = vertices.emplace_back();
        upperLeft.pos = vec2(upperLeftPos.x, 1.0f - upperLeftPos.y);
        upperLeft.uv = vec2(0, 0);

        UISystem::UIVertex& upperRight = vertices.emplace_back();
        upperRight.pos = vec2(upperRightPos.x, 1.0f - upperRightPos.y);
        upperRight.uv = vec2(1, 0);

        UISystem::UIVertex& lowerLeft = vertices.emplace_back();
        lowerLeft.pos = vec2(lowerLeftPos.x, 1.0f - lowerLeftPos.y);
        lowerLeft.uv = vec2(0, 1);

        UISystem::UIVertex& lowerRight = vertices.emplace_back();
        lowerRight.pos = vec2(lowerRightPos.x, 1.0f - lowerRightPos.y);
        lowerRight.uv = vec2(1, 1);
    }

    void UpdateElementSystem::Update(entt::registry& registry)
    {
        Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

        auto& dataSingleton = registry.ctx<UISingleton::UIDataSingleton>();
        // Destroy elements queued for destruction.
        {
            size_t deleteEntityNum = dataSingleton.destructionQueue.size_approx();
            std::vector<entt::entity> deleteEntities;
            deleteEntities.reserve(deleteEntityNum);

            dataSingleton.destructionQueue.try_dequeue_bulk(deleteEntities.begin(), deleteEntityNum);
            for (entt::entity entId : deleteEntities)
            {
                delete dataSingleton.entityToElement[entId];
            }

            registry.destroy(deleteEntities.begin(), deleteEntities.end());
        }
        // Toggle visibility of elements
        {
            entt::entity entId;
            while (dataSingleton.visibilityToggleQueue.try_dequeue(entId))
            {
                if (registry.has<UIComponent::Visible>(entId))
                    registry.remove<UIComponent::Visible>(entId);
                else
                    registry.emplace<UIComponent::Visible>(entId);
            }
        }
        // Toggle collision of elements
        {
            entt::entity entId;
            while (dataSingleton.collisionToggleQueue.try_dequeue(entId))
            {
                if (registry.has<UIComponent::Collidable>(entId))
                    registry.remove<UIComponent::Collidable>(entId);
                else
                    registry.emplace<UIComponent::Collidable>(entId);
            }
        }

        auto boundsUpdateView = registry.view<UIComponent::Transform, UIComponent::BoundsDirty>();
        boundsUpdateView.each([&](UIComponent::Transform& transform)
            {
                UIUtils::Transform::UpdateBounds(ServiceLocator::GetUIRegistry(), &transform, true);
            });

        auto imageView = registry.view<UIComponent::Transform, UIComponent::Image, UIComponent::Dirty>();
        imageView.each([&](UIComponent::Transform& transform, UIComponent::Image& image)
        {
            ZoneScopedNC("UpdateElementSystem::Update::ImageView", tracy::Color::RoyalBlue);

            // Renderable Updates
            if (image.texture.length() == 0)
                return;

            // (Re)load texture
            {
                ZoneScopedNC("(Re)load Texture", tracy::Color::RoyalBlue);
                image.textureID = renderer->LoadTexture(Renderer::TextureDesc{ image.texture });
            }

            // Create constant buffer if necessary
            auto constantBuffer = image.constantBuffer;
            if (constantBuffer == nullptr)
            {
                constantBuffer = new Renderer::Buffer<UIComponent::Image::ImageConstantBuffer>(renderer, "UpdateElementSystemConstantBuffer", Renderer::BUFFER_USAGE_UNIFORM_BUFFER, Renderer::BufferCPUAccess::WriteOnly);
                image.constantBuffer = constantBuffer;
            }
            constantBuffer->resource.color = image.color;
            constantBuffer->ApplyAll();

            // Transform Updates.
            const vec2& pos = UIUtils::Transform::GetMinBounds(&transform);
            const vec2& size = transform.size;

            std::vector<UISystem::UIVertex> vertices;
            CalculateVertices(pos, size, vertices);

            static const u32 bufferSize = sizeof(UISystem::UIVertex) * 4; // 4 vertices per image

            if (image.vertexBufferID == Renderer::BufferID::Invalid())
            {
                Renderer::BufferDesc desc;
                desc.name = "ImageVertices";
                desc.size = bufferSize;
                desc.usage = Renderer::BufferUsage::BUFFER_USAGE_UNIFORM_BUFFER;
                desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

                image.vertexBufferID = renderer->CreateBuffer(desc);
            }

            void* dst = renderer->MapBuffer(image.vertexBufferID);
            memcpy(dst, vertices.data(), bufferSize);
            renderer->UnmapBuffer(image.vertexBufferID);
        });

        auto textView = registry.view<UIComponent::Transform, UIComponent::Text, UIComponent::Dirty>();
        textView.each([&](UIComponent::Transform& transform, UIComponent::Text& text)
        {
            ZoneScopedNC("UpdateElementSystem::Update::TextView", tracy::Color::SkyBlue);
            if (text.fontPath.length() == 0)
                return;

            {
                ZoneScopedNC("(Re)load Font", tracy::Color::RoyalBlue);
                text.font = Renderer::Font::GetFont(renderer, text.fontPath, text.fontSize);
            }
            
            std::vector<f32> lineWidths;
            std::vector<size_t> lineBreakPoints;
            size_t finalCharacter = UIUtils::Text::CalculateLineWidthsAndBreaks(&text, transform.size.x, transform.size.y, lineWidths, lineBreakPoints);

            size_t textLengthWithoutSpaces = std::count_if(text.text.begin() + text.pushback, text.text.end() - (text.text.length() - finalCharacter), [](char c) { return !std::isspace(c); });

            // If textLengthWithoutSpaces is bigger than the amount of glyphs we allocated in our buffer we need to reallocate the buffer
            static const u32 perGlyphVertexSize = sizeof(UISystem::UIVertex) * 4; // 4 vertices per glyph
            if (textLengthWithoutSpaces > text.vertexBufferGlyphCount)
            {
                if (text.vertexBufferID != Renderer::BufferID::Invalid())
                {
                    renderer->QueueDestroyBuffer(text.vertexBufferID);
                }
                if (text.textureIDBufferID != Renderer::BufferID::Invalid())
                {
                    renderer->QueueDestroyBuffer(text.textureIDBufferID);
                }

                Renderer::BufferDesc vertexBufferDesc;
                vertexBufferDesc.name = "TextView";
                vertexBufferDesc.size = textLengthWithoutSpaces * perGlyphVertexSize;
                vertexBufferDesc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER;
                vertexBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

                text.vertexBufferID = renderer->CreateBuffer(vertexBufferDesc);

                Renderer::BufferDesc textureIDBufferDesc;
                textureIDBufferDesc.name = "TexturesIDs";
                textureIDBufferDesc.size = textLengthWithoutSpaces * sizeof(u32); // 1 u32 per glyph
                textureIDBufferDesc.usage = Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER;
                textureIDBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;

                text.textureIDBufferID = renderer->CreateBuffer(textureIDBufferDesc);

                text.vertexBufferGlyphCount = textLengthWithoutSpaces;
            }
            
            text.glyphCount = textLengthWithoutSpaces;

            f32 horizontalAlignment = UIUtils::Text::GetHorizontalAlignment(text.horizontalAlignment);
            f32 verticalAlignment = UIUtils::Text::GetVerticalAlignment(text.verticalAlignment);
            vec2 currentPosition = UIUtils::Transform::GetAnchorPosition(&transform, vec2(horizontalAlignment, verticalAlignment));
            f32 startX = currentPosition.x;
            currentPosition.x -= lineWidths[0] * horizontalAlignment;
            currentPosition.y += text.fontSize * (1 - verticalAlignment);

            if (textLengthWithoutSpaces > 0)
            {
                std::vector<UISystem::UIVertex> vertices;

                UISystem::UIVertex* baseVertices = reinterpret_cast<UISystem::UIVertex*>(renderer->MapBuffer(text.vertexBufferID));
                u32* baseTextureID = reinterpret_cast<u32*>(renderer->MapBuffer(text.textureIDBufferID));

                size_t currentLine = 0;
                size_t glyph = 0;
                for (size_t i = text.pushback; i < finalCharacter; i++)
                {
                    const char character = text.text[i];
                    if (currentLine < lineBreakPoints.size() && lineBreakPoints[currentLine] == i)
                    {
                        currentLine++;
                        currentPosition.y += text.fontSize * text.lineHeight;
                        currentPosition.x = startX - lineWidths[currentLine] * horizontalAlignment;
                    }

                    if (character == '\n')
                    {
                        continue;
                    }
                    else if (std::isspace(character))
                    {
                        currentPosition.x += text.fontSize * 0.15f;
                        continue;
                    }

                    const Renderer::FontChar& fontChar = text.font->GetChar(character);
                    const vec2& pos = currentPosition + vec2(fontChar.xOffset, fontChar.yOffset);
                    const vec2& size = vec2(fontChar.width, fontChar.height);

                    vertices.clear();
                    CalculateVertices(pos, size, vertices);

                    UISystem::UIVertex* dst = &baseVertices[glyph * 4]; // 4 vertices per glyph
                    memcpy(dst, vertices.data(), perGlyphVertexSize);
                    baseTextureID[glyph] = fontChar.textureIndex;

                    currentPosition.x += fontChar.advance;
                    glyph++;
                }

                renderer->UnmapBuffer(text.vertexBufferID);
                renderer->UnmapBuffer(text.textureIDBufferID);
            }

            // Create constant buffer if necessary
            if (!text.constantBuffer)
                text.constantBuffer = new Renderer::Buffer<UIComponent::Text::TextConstantBuffer>(renderer, "UpdateElementSystemConstantBuffer", Renderer::BUFFER_USAGE_UNIFORM_BUFFER, Renderer::BufferCPUAccess::WriteOnly);

            text.constantBuffer->resource.textColor = text.color;
            text.constantBuffer->resource.outlineColor = text.outlineColor;
            text.constantBuffer->resource.outlineWidth = text.outlineWidth;
            text.constantBuffer->Apply(0);
            text.constantBuffer->Apply(1);
        });

        registry.clear<UIComponent::Dirty>();
        registry.clear<UIComponent::BoundsDirty>();
    }
}