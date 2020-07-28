#include "UpdateElementSystem.h"
#include <entity/registry.hpp>
#include <tracy/Tracy.hpp>
#include "../../../Utils/ServiceLocator.h"

#include "../../Components/UI/UITransform.h"
#include "../../Components/UI/UIImage.h"
#include "../../Components/UI/UIText.h"
#include "../../Components/UI/UIDirty.h"
#include "../../Components/UI/Singletons/UIDataSingleton.h"
#include "../../../UI/TransformUtils.h"
#include "../../../UI/TextUtils.h"

#include "../../render-lib/Renderer/Descriptors/ModelDesc.h"

void CalculateVertices(const vec2& pos, const vec2& size, std::vector<Renderer::Vertex>& vertices)
{
    const UI::UIDataSingleton& dataSingleton = ServiceLocator::GetUIRegistry()->ctx<UI::UIDataSingleton>();

    vec3 upperLeftPos = vec3(pos.x, pos.y, 0.f);
    vec3 upperRightPos = vec3(pos.x + size.x, pos.y, 0.f);
    vec3 lowerLeftPos = vec3(pos.x, pos.y + size.y, 0.f);
    vec3 lowerRightPos = vec3(pos.x + size.x, pos.y + size.y, 0.f);

    // UV space
    // TODO: Do scaling depending on rendertargets actual size instead of assuming 1080p (which is our reference resolution)
    upperLeftPos /= vec3(dataSingleton.UIRESOLUTION, 1.f);
    upperRightPos /= vec3(dataSingleton.UIRESOLUTION, 1.f);
    lowerLeftPos /= vec3(dataSingleton.UIRESOLUTION, 1.f);
    lowerRightPos /= vec3(dataSingleton.UIRESOLUTION, 1.f);

    // Vertices
    Renderer::Vertex upperLeft;
    upperLeft.pos = upperLeftPos;
    upperLeft.normal = vec3(0, 1, 0);
    upperLeft.texCoord = vec2(0, 0);

    Renderer::Vertex upperRight;
    upperRight.pos = upperRightPos;
    upperRight.normal = vec3(0, 1, 0);
    upperRight.texCoord = vec2(1, 0);

    Renderer::Vertex lowerLeft;
    lowerLeft.pos = lowerLeftPos;
    lowerLeft.normal = vec3(0, 1, 0);
    lowerLeft.texCoord = vec2(0, 1);

    Renderer::Vertex lowerRight;
    lowerRight.pos = lowerRightPos;
    lowerRight.normal = vec3(0, 1, 0);
    lowerRight.texCoord = vec2(1, 1);

    vertices.reserve(4);
    vertices.push_back(upperLeft);
    vertices.push_back(upperRight);
    vertices.push_back(lowerLeft);
    vertices.push_back(lowerRight);
}

void UpdateElementSystem::Update(entt::registry& registry)
{
    Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

    auto imageView = registry.view<UITransform, UIImage, UIDirty>();
    imageView.each([&](UITransform& transform, UIImage& image)
        {
            ZoneScopedNC("UpdateElementSystem::Update::ImageView", tracy::Color::RoyalBlue)

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
                constantBuffer = renderer->CreateConstantBuffer<UIImage::ImageConstantBuffer>();
                image.constantBuffer = constantBuffer;
            }
            constantBuffer->resource.color = image.color;
            constantBuffer->ApplyAll();

            // Transform Updates.
            const vec2& pos = UI::TransformUtils::GetMinBounds(transform);
            const vec2& size = transform.size;

            // Update vertex buffer
            Renderer::PrimitiveModelDesc primitiveModelDesc;
            CalculateVertices(pos, size, primitiveModelDesc.vertices);

            // If the primitive model hasn't been created yet, create it
            if (image.modelID == Renderer::ModelID::Invalid())
            {
                // Indices
                primitiveModelDesc.indices.push_back(0);
                primitiveModelDesc.indices.push_back(1);
                primitiveModelDesc.indices.push_back(2);
                primitiveModelDesc.indices.push_back(1);
                primitiveModelDesc.indices.push_back(3);
                primitiveModelDesc.indices.push_back(2);

                image.modelID = renderer->CreatePrimitiveModel(primitiveModelDesc);
            }
            else // Otherwise we just update the already existing primitive model
            {
                renderer->UpdatePrimitiveModel(image.modelID, primitiveModelDesc);
            }
        });

    auto textView = registry.view<UITransform, UI::UIText, UIDirty>();
    textView.each([&](UITransform& transform, UI::UIText& text)
        {
            ZoneScopedNC("UpdateElementSystem::Update::TextView", tracy::Color::SkyBlue)
            if (text.fontPath.length() == 0)
                return;

            text.font = Renderer::Font::GetFont(renderer, text.fontPath, text.fontSize);

            std::vector<f32> lineWidths;
            std::vector<size_t> lineBreakPoints;
            size_t finalCharacter = UI::TextUtils::CalculateLineWidthsAndBreaks(text, transform.size.x, transform.size.y, lineWidths, lineBreakPoints);

            size_t textLengthWithoutSpaces = std::count_if(text.text.begin() + text.pushback, text.text.end() - (text.text.length() - finalCharacter), [](char c) { return !std::isspace(c); });
            if (text.models.size() < textLengthWithoutSpaces)
            {
                size_t difference = textLengthWithoutSpaces - text.glyphCount;
                text.models.insert(text.models.end(), difference, Renderer::ModelID::Invalid());
                text.textures.insert(text.textures.end(), difference, Renderer::TextureID::Invalid());
            }
            text.glyphCount = textLengthWithoutSpaces;

            f32 horizontalAlignment = UI::TextUtils::GetHorizontalAlignment(text.horizontalAlignment);
            f32 verticalAlignment = UI::TextUtils::GetVerticalAlignment(text.verticalAlignment);
            vec2 currentPosition = UI::TransformUtils::GetAnchorPosition(transform, vec2(horizontalAlignment, verticalAlignment));
            f32 startX = currentPosition.x;
            currentPosition.x -= lineWidths[0] * horizontalAlignment;
            currentPosition.y += text.fontSize * (1 - verticalAlignment);

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

                Renderer::PrimitiveModelDesc primitiveModelDesc;
                primitiveModelDesc.debugName = "Text " + character;

                CalculateVertices(pos, size, primitiveModelDesc.vertices);

                Renderer::ModelID& modelID = text.models[glyph];

                // If the primitive model hasn't been created yet, create it
                if (modelID == Renderer::ModelID::Invalid())
                {
                    // Indices
                    primitiveModelDesc.indices.push_back(0);
                    primitiveModelDesc.indices.push_back(1);
                    primitiveModelDesc.indices.push_back(2);
                    primitiveModelDesc.indices.push_back(1);
                    primitiveModelDesc.indices.push_back(3);
                    primitiveModelDesc.indices.push_back(2);

                    modelID = renderer->CreatePrimitiveModel(primitiveModelDesc);
                }
                else // Otherwise we just update the already existing primitive model
                {
                    renderer->UpdatePrimitiveModel(modelID, primitiveModelDesc);
                }

                text.textures[glyph] = fontChar.texture;

                currentPosition.x += fontChar.advance;
                glyph++;
            }

            // Create constant buffer if necessary
            if (!text.constantBuffer)
                text.constantBuffer = renderer->CreateConstantBuffer<UI::UIText::TextConstantBuffer>();

            text.constantBuffer->resource.textColor = text.color;
            text.constantBuffer->resource.outlineColor = text.outlineColor;
            text.constantBuffer->resource.outlineWidth = text.outlineWidth;
            text.constantBuffer->Apply(0);
            text.constantBuffer->Apply(1);
        });

    registry.clear<UIDirty>();
}
