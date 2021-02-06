#include "UpdateTextModelSystem.h"
#include <entity/registry.hpp>
#include "../../UITypes.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../render-lib/Renderer/Renderer.h"

#include "../Components/Singletons/UIDataSingleton.h"
#include "../Components/Transform.h"
#include "../Components/InputField.h"
#include "../Components/Text.h"
#include "../Components/Dirty.h"

#include "../../Utils/TransformUtils.h"
#include "../../Utils/TextUtils.h"


namespace UISystem
{
    inline void CalculateVertices(const vec2& pos, const vec2& size, const UI::FBox& texCoords, std::array<UI::UIVertex, 4>& vertices)
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

    void UpdateTextModelSystem::Update(entt::registry& registry)
    {
        Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

        auto inputFieldView = registry.view<UIComponent::Transform, UIComponent::InputField, UIComponent::Text, UIComponent::Dirty>();
        inputFieldView.each([&](const UIComponent::Transform& transform, const UIComponent::InputField& inputField, UIComponent::Text& text)
        {
            text.pushback = UIUtils::Text::CalculatePushback(&text, inputField.writeHeadIndex, 0.2f, transform.size.x, transform.size.y);
        });

        auto textView = registry.view<UIComponent::Transform, UIComponent::Text, UIComponent::Dirty>();
        textView.each([&](UIComponent::Transform& transform, UIComponent::Text& text)
        {
            ZoneScopedNC("UpdateRenderingSystem::Update::TextView", tracy::Color::SkyBlue);
            if (text.style.fontPath.length() == 0)
                return;

            std::vector<f32> lineWidths;
            std::vector<size_t> lineBreakPoints;
            const size_t finalCharacter = UIUtils::Text::CalculateLineWidthsAndBreaks(&text, transform.size.x, transform.size.y, lineWidths, lineBreakPoints);
            const size_t textLengthWithoutSpaces = std::count_if(text.text.begin() + text.pushback, text.text.end() - (text.text.length() - finalCharacter), [](char c) { return !std::isspace(c); });

            // If textLengthWithoutSpaces is bigger than the amount of glyphs we allocated in our buffer we need to reallocate the buffer
            constexpr u32 perGlyphVertexSize = sizeof(UI::UIVertex) * 4; // 4 vertices per glyph
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

                Renderer::BufferDesc vertexBufferDesc { "TextView", Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER, Renderer::BufferCPUAccess::WriteOnly };
                vertexBufferDesc.size = textLengthWithoutSpaces * perGlyphVertexSize;

                text.vertexBufferID = renderer->CreateBuffer(vertexBufferDesc);

                Renderer::BufferDesc textureIDBufferDesc { "TexturesIDs", Renderer::BufferUsage::BUFFER_USAGE_STORAGE_BUFFER, Renderer::BufferCPUAccess::WriteOnly };
                textureIDBufferDesc.size = textLengthWithoutSpaces * sizeof(u32); // 1 u32 per glyph

                text.textureIDBufferID = renderer->CreateBuffer(textureIDBufferDesc);

                text.vertexBufferGlyphCount = textLengthWithoutSpaces;
            }
            text.glyphCount = textLengthWithoutSpaces;

            if (textLengthWithoutSpaces > 0)
            {
                const vec2 alignment = UIUtils::Text::GetAlignment(&text);
                vec2 currentPosition = UIUtils::Transform::GetAnchorPositionInElement(transform, alignment);
                const f32 startX = currentPosition.x;
                currentPosition.x -= lineWidths[0] * alignment.x;
                currentPosition.y += text.style.fontSize * (1 - alignment.y) * lineWidths.size();

                UI::UIVertex* baseVertices = reinterpret_cast<UI::UIVertex*>(renderer->MapBuffer(text.vertexBufferID));
                u32* baseTextureID = reinterpret_cast<u32*>(renderer->MapBuffer(text.textureIDBufferID));

                size_t line = 0, glyph = 0;
                for (size_t i = text.pushback; i < finalCharacter; i++)
                {
                    const char character = text.text[i];
                    if (line < lineBreakPoints.size() && lineBreakPoints[line] == i)
                    {
                        line++;
                        currentPosition.y += text.style.fontSize * text.style.lineHeightMultiplier;
                        currentPosition.x = startX - lineWidths[line] * alignment.x;
                    }

                    if (character == '\n')
                    {
                        continue;
                    }
                    else if (std::isspace(character))
                    {
                        currentPosition.x += text.style.fontSize * 0.15f;
                        continue;
                    }

                    const Renderer::FontChar& fontChar = text.font->GetChar(character);
                    const vec2 pos = currentPosition + vec2(fontChar.xOffset, fontChar.yOffset);
                    const vec2 size = vec2(fontChar.width, fontChar.height);
                    constexpr UI::FBox texCoords{ 0.f, 1.f, 1.f, 0.f };

                    std::array<UI::UIVertex, 4> vertices;
                    CalculateVertices(pos, size, texCoords, vertices);

                    UI::UIVertex* dst = &baseVertices[glyph * 4]; // 4 vertices per glyph
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

            text.constantBuffer->resource.textColor = text.style.color;
            text.constantBuffer->resource.outlineColor = text.style.outlineColor;
            text.constantBuffer->resource.outlineWidth = text.style.outlineWidth;
            text.constantBuffer->Apply(0);
            text.constantBuffer->Apply(1);
        });
    }
}