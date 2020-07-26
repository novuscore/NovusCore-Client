#include "UIRenderer.h"

#include <Renderer/Renderer.h>
#include <Renderer/Renderers/Vulkan/RendererVK.h>
#include <Renderer/Descriptors/FontDesc.h>
#include <Window/Window.h>
#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>

#include "../Utils/ServiceLocator.h"
#include "../UI/TransformUtils.h"
#include "../UI/TextUtils.h"

#include "../ECS/Components/UI/Singletons/UIEntityPoolSingleton.h"
#include "../ECS/Components/UI/Singletons/UIAddElementQueueSingleton.h"
#include "../ECS/Components/UI/Singletons/UIDataSingleton.h"

#include "../ECS/Components/UI/UITransform.h"
#include "../ECS/Components/UI/UITransformEvents.h"

#include "../ECS/Components/UI/UIRenderable.h"
#include "../ECS/Components/UI/UIImage.h"
#include "../ECS/Components/UI/UIText.h"

#include "../ECS/Components/UI/UIVisible.h"
#include "../ECS/Components/UI/UIDirty.h"
#include "../ECS/Components/UI/UICollidable.h"

#include "../ECS/Components/UI/UIInputField.h"
#include "../ECS/Components/UI/UICheckbox.h"

#include "../Scripting/Classes/UI/asInputfield.h"
#include "../Scripting/Classes/UI/asCheckbox.h"

const u32 WIDTH = 1920;
const u32 HEIGHT = 1080;

UIRenderer::UIRenderer(Renderer::Renderer* renderer) : _renderer(renderer)
{
    CreatePermanentResources();

    InputManager* inputManager = ServiceLocator::GetInputManager();
    inputManager->RegisterKeybind("UI Click Checker", GLFW_MOUSE_BUTTON_LEFT, KEYBIND_ACTION_CLICK, KEYBIND_MOD_ANY, std::bind(&UIRenderer::OnMouseClick, this, std::placeholders::_1, std::placeholders::_2));
    inputManager->RegisterMousePositionCallback("UI Mouse Position Checker", std::bind(&UIRenderer::OnMousePositionUpdate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    inputManager->RegisterKeyboardInputCallback("UI Keyboard Input Checker"_h, std::bind(&UIRenderer::OnKeyboardInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    inputManager->RegisterCharInputCallback("UI Char Input Checker"_h, std::bind(&UIRenderer::OnCharInput, this, std::placeholders::_1, std::placeholders::_2));

    entt::registry* registry = ServiceLocator::GetUIRegistry();
    registry->prepare<UITransform>();
    registry->prepare<UITransformEvents>();
    registry->prepare<UIRenderable>();
    registry->prepare<UIImage>();
    registry->prepare<UI::UIText>();

    registry->prepare<UIVisible>();
    registry->prepare<UIVisibility>();

    registry->prepare<UIDirty>();
    registry->prepare<UICollidable>();

    registry->prepare<UIInputField>();
    registry->prepare<UICheckbox>();

    // Register UI singletons.
    registry->set<UI::UIDataSingleton>();
    registry->set<UI::UIAddElementQueueSingleton>();

    // Register entity pool.
    registry->set<UI::UIEntityPoolSingleton>().AllocatePool();
}

void UIRenderer::Update(f32 deltaTime)
{
    ZoneScoped;
    entt::registry* registry = ServiceLocator::GetUIRegistry();

    auto imageView = registry->view<UITransform, UIImage, UIDirty>();
    imageView.each([this, registry](const auto entity, UITransform& transform, UIImage& image)
        {
            ZoneScoped;
            // Renderable Updates
            if (image.texture.length() == 0)
                return;

            // (Re)load texture
            image.textureID = ReloadTexture(image.texture);

            // Create constant buffer if necessary
            auto constantBuffer = image.constantBuffer;
            if (constantBuffer == nullptr)
            {
                constantBuffer = _renderer->CreateConstantBuffer<UIImage::ImageConstantBuffer>();
                image.constantBuffer = constantBuffer;
            }
            constantBuffer->resource.color = image.color;
            constantBuffer->ApplyAll();

            // Transform Updates.
            const vec2& pos = vec2(UI::TransformUtils::GetMinBounds(transform));
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

                image.modelID = _renderer->CreatePrimitiveModel(primitiveModelDesc);
            }
            else // Otherwise we just update the already existing primitive model
            {
                _renderer->UpdatePrimitiveModel(image.modelID, primitiveModelDesc);
            }
        });

    auto textView = registry->view<UITransform, UI::UIText, UIDirty>();
    textView.each([this, registry](const auto entity, UITransform& transform, UI::UIText& text)
        {
            ZoneScoped;
            if (text.fontPath.length() == 0)
                return;

            text.font = Renderer::Font::GetFont(_renderer, text.fontPath, text.fontSize);

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

                    modelID = _renderer->CreatePrimitiveModel(primitiveModelDesc);
                }
                else // Otherwise we just update the already existing primitive model
                {
                    _renderer->UpdatePrimitiveModel(modelID, primitiveModelDesc);
                }

                text.textures[glyph] = fontChar.texture;

                currentPosition.x += fontChar.advance;
                glyph++;
            }

            // Create constant buffer if necessary
            if (!text.constantBuffer)
                text.constantBuffer = _renderer->CreateConstantBuffer<UI::UIText::TextConstantBuffer>();

            text.constantBuffer->resource.textColor = text.color;
            text.constantBuffer->resource.outlineColor = text.outlineColor;
            text.constantBuffer->resource.outlineWidth = text.outlineWidth;
            text.constantBuffer->Apply(0);
            text.constantBuffer->Apply(1);
        });

    registry->clear<UIDirty>();
}

void UIRenderer::AddUIPass(Renderer::RenderGraph* renderGraph, Renderer::ImageID renderTarget, u8 frameIndex)
{
    // UI Pass

    struct UIPassData
    {
        Renderer::RenderPassMutableResource renderTarget;
    };

    renderGraph->AddPass<UIPassData>("UI Pass",
        [&](UIPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.renderTarget = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_LOAD);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
        [&](UIPassData& data, Renderer::CommandList& commandList) // Execute
        {
            ZoneScopedN("Renderer - UIPass");

            Renderer::GraphicsPipelineDesc pipelineDesc;
            renderGraph->InitializePipelineDesc(pipelineDesc);

            // Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
            pipelineDesc.states.inputLayouts[0].enabled = true;
            pipelineDesc.states.inputLayouts[0].SetName("POSITION");
            pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::INPUT_FORMAT_R32G32B32_FLOAT;
            pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
            pipelineDesc.states.inputLayouts[1].enabled = true;
            pipelineDesc.states.inputLayouts[1].SetName("NORMAL");
            pipelineDesc.states.inputLayouts[1].format = Renderer::InputFormat::INPUT_FORMAT_R32G32B32_FLOAT;
            pipelineDesc.states.inputLayouts[1].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
            pipelineDesc.states.inputLayouts[2].enabled = true;
            pipelineDesc.states.inputLayouts[2].SetName("TEXCOORD");
            pipelineDesc.states.inputLayouts[2].format = Renderer::InputFormat::INPUT_FORMAT_R32G32_FLOAT;
            pipelineDesc.states.inputLayouts[2].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;

            // Render targets
            pipelineDesc.renderTargets[0] = data.renderTarget;

            // Blending
            pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
            pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::BLEND_MODE_SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::BLEND_MODE_INV_SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].srcBlendAlpha = Renderer::BlendMode::BLEND_MODE_ZERO;
            pipelineDesc.states.blendState.renderTargets[0].destBlendAlpha = Renderer::BlendMode::BLEND_MODE_ONE;

            // Panel Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/panel.vs.hlsl.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "Data/shaders/panel.ps.hlsl.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            Renderer::GraphicsPipelineID imagePipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline

            // Text Shaders
            vertexShaderDesc.path = "Data/shaders/text.vs.hlsl.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            pixelShaderDesc.path = "Data/shaders/text.ps.hlsl.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            Renderer::GraphicsPipelineID textPipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline

            // Set pipeline
            commandList.BeginPipeline(imagePipeline);
            Renderer::GraphicsPipelineID activePipeline = imagePipeline;

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, frameIndex);

            {
                ZoneScopedNC("UIRenderer::AddUIPass - Render", tracy::Color::Red)

                    entt::registry* registry = ServiceLocator::GetUIRegistry();
                auto renderGroup = registry->group<UITransform>(entt::get<UIRenderable, UIVisible>);
                renderGroup.sort<UITransform>([](UITransform& first, UITransform& second) { return first.sortKey < second.sortKey; });
                renderGroup.each([this, &commandList, frameIndex, &registry, &activePipeline, &textPipeline, &imagePipeline](const auto entity, UITransform& transform)
                    {
                        switch (transform.sortData.type)
                        {
                        case UI::UIElementType::UITYPE_TEXT:
                        case UI::UIElementType::UITYPE_INPUTFIELD:
                        {
                            UI::UIText& text = registry->get<UI::UIText>(entity);
                            if (!text.constantBuffer)
                                return;

                            if (activePipeline != textPipeline)
                            {
                                commandList.EndPipeline(activePipeline);
                                commandList.BeginPipeline(textPipeline);
                                activePipeline = textPipeline;
                            }

                            commandList.PushMarker("Text", Color(0.0f, 0.1f, 0.0f));

                            // Bind textdata descriptor
                            _drawDescriptorSet.Bind("_textData"_h, text.constantBuffer);

                            // Each glyph in the label has it's own plane and texture, this could be optimized in the future.
                            for (u32 i = 0; i < text.glyphCount; i++)
                            {
                                // Bind texture descriptor
                                _drawDescriptorSet.Bind("_texture"_h, text.textures[i]);

                                commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawDescriptorSet, frameIndex);

                                // Draw
                                commandList.Draw(text.models[i]);
                            }

                            commandList.PopMarker();
                            break;
                        }
                        default:
                        {
                            UIImage& image = registry->get<UIImage>(entity);
                            if (!image.constantBuffer)
                                return;

                            if (activePipeline != imagePipeline)
                            {
                                commandList.EndPipeline(activePipeline);
                                commandList.BeginPipeline(imagePipeline);
                                activePipeline = imagePipeline;
                            }

                            commandList.PushMarker("Image", Color(0.0f, 0.1f, 0.0f));

                            // Bind descriptors
                            _drawDescriptorSet.Bind("_panelData"_h, image.constantBuffer);
                            _drawDescriptorSet.Bind("_texture"_h, image.textureID);

                            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawDescriptorSet, frameIndex);

                            // Draw
                            commandList.Draw(image.modelID);

                            commandList.PopMarker();
                            break;
                        }
                        }
                    });
                commandList.EndPipeline(activePipeline);
            }
        });
}

bool UIRenderer::OnMouseClick(Window* window, std::shared_ptr<Keybind> keybind)
{
    ZoneScoped;
    entt::registry* registry = ServiceLocator::GetUIRegistry();
    UI::UIDataSingleton& dataSingleton = registry->ctx<UI::UIDataSingleton>();

    const vec2 mouse = ServiceLocator::GetInputManager()->GetMousePosition();

    //Unfocus last focused widget.
    entt::entity lastFocusedWidget = dataSingleton.focusedWidget;
    if (dataSingleton.focusedWidget != entt::null)
    {
        registry->get<UITransformEvents>(dataSingleton.focusedWidget).OnUnfocused();

        dataSingleton.focusedWidget = entt::null;
    }

    auto eventGroup = registry->group<UITransformEvents>(entt::get<UITransform, UICollidable, UIVisible>);
    eventGroup.sort<UITransform>([](const UITransform& left, const UITransform& right) { return left.sortKey > right.sortKey; });
    for (auto entity : eventGroup)
    {
        const UITransform& transform = eventGroup.get<UITransform>(entity);
        UITransformEvents& events = eventGroup.get<UITransformEvents>(entity);

        const vec2 minBounds = transform.minBound;
        const vec2 maxBounds = transform.maxBound;

        // Check so mouse if within widget bounds.
        if (!((mouse.x > minBounds.x && mouse.x < maxBounds.x) && (mouse.y > minBounds.y && mouse.y < maxBounds.y)))
            continue;

        // Don't interact with the last focused widget directly again. The first click is reserved for unfocusing it. But we still need to block clicking through it.
        if (lastFocusedWidget == entity)
            return true;

        // Check if we have any events we can actually call else exit out early. It needs to still block clicking through though.
        if (!events.flags)
            return true;

        if (keybind->state == GLFW_PRESS)
        {
            if (events.IsDraggable())
            {
                // TODO FEATURE: Dragging
            }
        }
        else
        {
            if (events.IsFocusable())
            {
                dataSingleton.focusedWidget = entity;

                events.OnFocused();
            }

            if (events.IsClickable())
            {
                events.OnClick();

                if (transform.sortData.type == UI::UIElementType::UITYPE_CHECKBOX)
                {
                    UI::asCheckbox* checkBox = reinterpret_cast<UI::asCheckbox*>(transform.asObject);
                    checkBox->ToggleChecked();
                }
            }
        }

        return true;
    }

    return false;
}

void UIRenderer::OnMousePositionUpdate(Window* window, f32 x, f32 y)
{
    // TODO FEATURE: Handle Dragging
}

bool UIRenderer::OnKeyboardInput(Window* window, i32 key, i32 action, i32 modifiers)
{
    entt::registry* registry = ServiceLocator::GetUIRegistry();
    UI::UIDataSingleton& dataSingleton = registry->ctx<UI::UIDataSingleton>();

    if (dataSingleton.focusedWidget == entt::null || action != GLFW_RELEASE)
        return false;

    UITransform& transform = registry->get<UITransform>(dataSingleton.focusedWidget);
    UITransformEvents& events = registry->get<UITransformEvents>(dataSingleton.focusedWidget);

    if (key == GLFW_KEY_ESCAPE)
    {
        events.OnUnfocused();
        dataSingleton.focusedWidget = entt::null;

        return true;
    }

    switch (transform.sortData.type)
    {
    case UI::UIElementType::UITYPE_INPUTFIELD:
    {
        UI::asInputField* inputFieldAS = reinterpret_cast<UI::asInputField*>(transform.asObject);
        inputFieldAS->HandleKeyInput(key);
        break;
    }
    case UI::UIElementType::UITYPE_CHECKBOX:
    {
        UI::asCheckbox* checkBoxAS = reinterpret_cast<UI::asCheckbox*>(transform.asObject);
        checkBoxAS->HandleKeyInput(key);
        break;
    }
    default:
        if (key == GLFW_KEY_ENTER && events.IsClickable())
        {
            events.OnClick();
        }
        break;
    }

    return true;
}

bool UIRenderer::OnCharInput(Window* window, u32 unicodeKey)
{
    entt::registry* registry = ServiceLocator::GetUIRegistry();
    UI::UIDataSingleton& dataSingleton = registry->ctx<UI::UIDataSingleton>();

    if (dataSingleton.focusedWidget == entt::null)
        return false;

    UITransform& transform = registry->get<UITransform>(dataSingleton.focusedWidget);
    if (transform.sortData.type == UI::UIElementType::UITYPE_INPUTFIELD)
    {
        UI::asInputField* inputField = reinterpret_cast<UI::asInputField*>(transform.asObject);
        inputField->HandleCharInput((char)unicodeKey);
    }

    return true;
}

void UIRenderer::CreatePermanentResources()
{
    // Sampler
    Renderer::SamplerDesc samplerDesc;
    samplerDesc.enabled = true;
    samplerDesc.filter = Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.addressU = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressV = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressW = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_PIXEL;

    _linearSampler = _renderer->CreateSampler(samplerDesc);

    // Create descriptor sets
    _passDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _passDescriptorSet.Bind("_sampler"_h, _linearSampler);

    _drawDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
}

Renderer::TextureID UIRenderer::ReloadTexture(const std::string& texturePath)
{
    Renderer::TextureDesc textureDesc;
    textureDesc.path = texturePath;

    return _renderer->LoadTexture(textureDesc);
}

void UIRenderer::CalculateVertices(const vec2& pos, const vec2& size, std::vector<Renderer::Vertex>& vertices)
{
    vec3 upperLeftPos = vec3(pos.x, pos.y, 0.f);
    vec3 upperRightPos = vec3(pos.x + size.x, pos.y, 0.f);
    vec3 lowerLeftPos = vec3(pos.x, pos.y + size.y, 0.f);
    vec3 lowerRightPos = vec3(pos.x + size.x, pos.y + size.y, 0.f);

    // UV space
    // TODO: Do scaling depending on rendertargets actual size instead of assuming 1080p (which is our reference resolution)
    upperLeftPos /= vec3(WIDTH, HEIGHT, 1.0f);
    upperRightPos /= vec3(WIDTH, HEIGHT, 1.0f);
    lowerLeftPos /= vec3(WIDTH, HEIGHT, 1.0f);
    lowerRightPos /= vec3(WIDTH, HEIGHT, 1.0f);

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

    vertices.push_back(upperLeft);
    vertices.push_back(upperRight);
    vertices.push_back(lowerLeft);
    vertices.push_back(lowerRight);
}
