#include "UIRenderer.h"
#include "Camera.h"

#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/UI/Singletons/UIEntityPoolSingleton.h"
#include "../ECS/Components/UI/Singletons/UIAddElementQueueSingleton.h"
#include "../ECS/Components/UI/Singletons/UIDataSingleton.h"

#include "../ECS/Components/UI/UITransform.h"
#include "../ECS/Components/UI/UITransformUtils.h"
#include "../ECS/Components/UI/UITransformEvents.h"
#include "../ECS/Components/UI/UIRenderable.h"
#include "../ECS/Components/UI/UIVisible.h"
#include "../ECS/Components/UI/UIDirty.h"
#include "../ECS/Components/UI/UIText.h"
#include "../ECS/Components/UI/UIInputField.h"

#include "../Scripting/Classes/UI/asInputfield.h"

#include <Renderer/Renderer.h>
#include <Renderer/Renderers/Vulkan/RendererVK.h>
#include <Renderer/Descriptors/FontDesc.h>
#include <Window/Window.h>
#include <InputManager.h>
#include <GLFW/glfw3.h>

const u32 WIDTH = 1920;
const u32 HEIGHT = 1080;

UIRenderer::UIRenderer(Renderer::Renderer* renderer)
{
    _renderer = renderer;
    CreatePermanentResources();

    InputManager* inputManager = ServiceLocator::GetInputManager();
    inputManager->RegisterKeybind("UI Click Checker", GLFW_MOUSE_BUTTON_LEFT, KEYBIND_ACTION_CLICK, KEYBIND_MOD_ANY, std::bind(&UIRenderer::OnMouseClick, this, std::placeholders::_1, std::placeholders::_2));
    inputManager->RegisterMousePositionCallback("UI Mouse Position Checker", std::bind(&UIRenderer::OnMousePositionUpdate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    inputManager->RegisterKeyboardInputCallback("UI Keyboard Input Checker"_h, std::bind(&UIRenderer::OnKeyboardInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    inputManager->RegisterCharInputCallback("UI Char Input Cheker"_h, std::bind(&UIRenderer::OnCharInput, this, std::placeholders::_1, std::placeholders::_2));

    entt::registry* registry = ServiceLocator::GetUIRegistry();
    registry->prepare<UITransform>();
    registry->prepare<UITransformEvents>();

    registry->prepare<UIRenderable>();
    registry->prepare<UIText>();

    registry->prepare<UIVisible>();
    registry->prepare<UIVisiblity>();

    // Register UI singletons.
    registry->set<UI::UIDataSingleton>();
    registry->set<UI::UIAddElementQueueSingleton>();

    // Preallocate Entity Ids
    registry->set<UI::UIEntityPoolSingleton>().AllocatePool();

    _focusedWidget = entt::null;
}

void UIRenderer::ClearWidgets()
{
    entt::registry* registry = ServiceLocator::GetUIRegistry();
    UI::UIDataSingleton& uiDataSingleton = registry->ctx<UI::UIDataSingleton>();

    std::vector<entt::entity> entityIds = std::vector<entt::entity>();
    entityIds.reserve(uiDataSingleton.entityToAsObject.size());
    
    for (auto asObject : uiDataSingleton.entityToAsObject)
    {
        entityIds.push_back(asObject.first);

        delete asObject.second;
    }
    uiDataSingleton.entityToAsObject.clear();
    
    // Delete entities.
    registry->destroy(entityIds.begin(), entityIds.end());

    _focusedWidget = entt::null;
}

void UIRenderer::DestroyWidget(entt::entity entId)
{
    entt::registry* registry = ServiceLocator::GetUIRegistry();
    UI::UIDataSingleton& uiDataSingleton = registry->ctx<UI::UIDataSingleton>();

    if (auto itr = uiDataSingleton.entityToAsObject.find(entId); itr != uiDataSingleton.entityToAsObject.end())
    {
        delete itr->second;
    }

    registry->destroy(entId);
}

void UIRenderer::Update(f32 deltaTime)
{
    entt::registry* registry = ServiceLocator::GetUIRegistry();

    auto renderableView = registry->view<UITransform, UIRenderable, UIDirty>();
    renderableView.each([this, registry](const auto entity, UITransform& transform, UIRenderable& renderable)
        {
            // Renderable Updates
            if (renderable.texture.length() == 0)
            {
                renderable.textureID = Renderer::TextureID::Invalid();
                return;
            }

            // (Re)load texture
            renderable.textureID = ReloadTexture(renderable.texture);

            // Create constant buffer if necessary
            auto constantBuffer = renderable.constantBuffer;
            if (constantBuffer == nullptr)
            {
                constantBuffer = _renderer->CreateConstantBuffer<UIRenderable::RenderableConstantBuffer>();
                renderable.constantBuffer = constantBuffer;
            }
            constantBuffer->resource.color = renderable.color;
            constantBuffer->Apply(0);
            constantBuffer->Apply(1);

            // Transform Updates.
            const vec3& pos = vec3(UITransformUtils::GetMinBounds(transform), transform.depth);
            const vec2& size = transform.size;

            // Update vertex buffer
            Renderer::PrimitiveModelDesc primitiveModelDesc;
            CalculateVertices(pos, size, primitiveModelDesc.vertices);

            // If the primitive model hasn't been created yet, create it
            if (renderable.modelID == Renderer::ModelID::Invalid())
            {
                // Indices
                primitiveModelDesc.indices.push_back(0);
                primitiveModelDesc.indices.push_back(1);
                primitiveModelDesc.indices.push_back(2);
                primitiveModelDesc.indices.push_back(1);
                primitiveModelDesc.indices.push_back(3);
                primitiveModelDesc.indices.push_back(2);

                renderable.modelID = _renderer->CreatePrimitiveModel(primitiveModelDesc);
            }
            else // Otherwise we just update the already existing primitive model
            {
                _renderer->UpdatePrimitiveModel(renderable.modelID, primitiveModelDesc);
            }
        });
    registry->remove<UIDirty>(renderableView.begin(), renderableView.end());

    auto textView = registry->view<UITransform, UIText, UIDirty>();
    textView.each([this, registry](const auto entity, UITransform& transform, UIText& text)
        {
            if (text.fontPath.length() == 0)
            {
                text.font = nullptr;
                return;
            }

            text.font = Renderer::Font::GetFont(_renderer, text.fontPath, text.fontSize);

            size_t textLengthWithoutSpaces = std::count_if(text.text.begin(), text.text.end(), [](char c)
                {
                    return c != ' ';
                });

            size_t glyphCount = text.models.size();
            if (glyphCount != textLengthWithoutSpaces)
            {
                text.models.resize(textLengthWithoutSpaces);
                for (size_t i = glyphCount; i < textLengthWithoutSpaces; i++)
                {
                    text.models[i] = Renderer::ModelID::Invalid();
                }
            }
            if (text.textures.size() != textLengthWithoutSpaces)
            {
                text.textures.resize(textLengthWithoutSpaces);
                for (size_t i = glyphCount; i < textLengthWithoutSpaces; i++)
                {
                    text.textures[i] = Renderer::TextureID::Invalid();
                }
            }

            vec3 currentPosition = vec3(UITransformUtils::GetMinBounds(transform), transform.depth);
            currentPosition.y -= text.font->GetChar('A').yOffset * 1.15f; //TODO Get line height in a less hacky way. (This was done to make text anchors also be the top-left as other widgets)

            size_t glyph = 0;
            for (char character : text.text)
            {
                if (character == ' ')
                {
                    currentPosition.x += text.fontSize * 0.15f;
                    continue;
                }

                Renderer::FontChar fontChar = text.font->GetChar(character);

                const vec3& pos = currentPosition + vec3(fontChar.xOffset, fontChar.yOffset, 0);
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
            if (text.constantBuffer == nullptr)
            {
                text.constantBuffer = _renderer->CreateConstantBuffer<UIText::TextConstantBuffer>();
            }
            text.constantBuffer->resource.textColor = text.color;
            text.constantBuffer->resource.outlineColor = text.outlineColor;
            text.constantBuffer->resource.outlineWidth = text.outlineWidth;
            text.constantBuffer->Apply(0);
            text.constantBuffer->Apply(1);
        });
    registry->remove<UIDirty>(textView.begin(), textView.end());
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
            Renderer::GraphicsPipelineDesc pipelineDesc;
            renderGraph->InitializePipelineDesc(pipelineDesc);

            // Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/panel.vert.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "Data/shaders/panel.frag.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

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

            // Viewport
            pipelineDesc.states.viewport.topLeftX = 0;
            pipelineDesc.states.viewport.topLeftY = 0;
            pipelineDesc.states.viewport.width = static_cast<f32>(WIDTH);
            pipelineDesc.states.viewport.height = static_cast<f32>(HEIGHT);
            pipelineDesc.states.viewport.minDepth = 0.0f;
            pipelineDesc.states.viewport.maxDepth = 1.0f;

            // ScissorRect
            pipelineDesc.states.scissorRect.left = 0;
            pipelineDesc.states.scissorRect.right = WIDTH;
            pipelineDesc.states.scissorRect.top = 0;
            pipelineDesc.states.scissorRect.bottom = HEIGHT;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;

            // Samplers TODO: We don't care which samplers we have here, we just need the number of samplers
            pipelineDesc.states.samplers[0].enabled = true;

            // Textures TODO: We don't care which textures we have here, we just need the number of textures
            pipelineDesc.textures[0] = Renderer::RenderPassResource(1);

            // Render targets
            pipelineDesc.renderTargets[0] = data.renderTarget;

            // Blending
            pipelineDesc.states.blendState.renderTargets[0].blendEnable = true;
            pipelineDesc.states.blendState.renderTargets[0].srcBlend = Renderer::BlendMode::BLEND_MODE_SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].destBlend = Renderer::BlendMode::BLEND_MODE_INV_SRC_ALPHA;
            pipelineDesc.states.blendState.renderTargets[0].srcBlendAlpha = Renderer::BlendMode::BLEND_MODE_ZERO;
            pipelineDesc.states.blendState.renderTargets[0].destBlendAlpha = Renderer::BlendMode::BLEND_MODE_ONE;

            // Set pipeline
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            // Draw all the panels
            entt::registry* registry = ServiceLocator::GetUIRegistry();
            auto renderableView = registry->view<UITransform, UIRenderable, UIVisible>();
            renderableView.each([this, &commandList, frameIndex](const auto, UITransform& transform, UIRenderable& renderable)
                {
                    if (renderable.textureID == Renderer::TextureID::Invalid() || !renderable.constantBuffer)
                        return;

                    commandList.PushMarker("Renderable", Color(0.0f, 0.1f, 0.0f));

                    // Set constant buffer
                    commandList.SetConstantBuffer(0, renderable.constantBuffer->GetDescriptor(frameIndex), frameIndex);

                    // Set Sampler and texture.
                    commandList.SetSampler(1, _linearSampler);
                    commandList.SetTexture(2, renderable.textureID);

                    // Draw
                    commandList.Draw(renderable.modelID);

                    commandList.PopMarker();
                });
            commandList.EndPipeline(pipeline);

            // Draw text
            vertexShaderDesc.path = "Data/shaders/text.vert.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            pixelShaderDesc.path = "Data/shaders/text.frag.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            // Set pipeline
            pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            auto textView = registry->view<UITransform, UIText, UIVisible>();
            textView.each([this, &commandList, frameIndex](const auto, UITransform& transform, UIText& text)
                {
                    if (!text.font || !text.constantBuffer)
                        return;

                    commandList.PushMarker("Text", Color(0.0f, 0.1f, 0.0f));

                    // Set constant buffer
                    commandList.SetConstantBuffer(0, text.constantBuffer->GetDescriptor(frameIndex), frameIndex);

                    // Set sampler
                    commandList.SetSampler(1, _linearSampler);

                    // Each glyph in the label has it's own plane and texture, this could be optimized in the future.
                    size_t glyphs = text.models.size();
                    for (u32 i = 0; i < glyphs; i++)
                    {
                        // Set texture
                        commandList.SetTexture(2, text.textures[i]);

                        // Draw
                        commandList.Draw(text.models[i]);
                    }

                    commandList.PopMarker();
                });

            commandList.EndPipeline(pipeline);
        });
}

bool UIRenderer::OnMouseClick(Window* window, std::shared_ptr<Keybind> keybind)
{
    entt::registry* registry = ServiceLocator::GetUIRegistry();
    const vec2 mouse = ServiceLocator::GetInputManager()->GetMousePosition();

    //Unfocus last focused widget.
    entt::entity lastFocusedWidget = _focusedWidget;
    if (_focusedWidget != entt::null)
    {
        auto& events = registry->get<UITransformEvents>(_focusedWidget);
        events.OnUnfocused();

        _focusedWidget = entt::null;
    }

    //TODO IMPROVEMENT: Depth sorting widgets.
    auto eventView = registry->group<UITransform, UITransformEvents, UIVisible>();
    eventView.sort<UITransform>([](UITransform& left, UITransform& right) { 
        return left.depth < right.depth; 
        });

    for (auto entity : eventView)
    {
        const UITransform& transform = eventView.get<UITransform>(entity);
        const vec2 minBounds = UITransformUtils::GetMinBounds(transform);
        const vec2 maxBounds = UITransformUtils::GetMaxBounds(transform);

        if ((mouse.x > minBounds.x && mouse.x < maxBounds.x) &&
            (mouse.y > minBounds.y && mouse.y < maxBounds.y))
        {
            UITransformEvents& events = eventView.get<UITransformEvents>(entity);

            // Check if we have any events we can actually call else exit out early. It needs to still block clicking through though.
            if (!events.flags)
                return true;

            // Don't interact with the last focused widget directly again. The first click is reserved for unfocusing it. But we still need to block clicking through it.
            if (lastFocusedWidget == entity)
                return true;

            if (keybind->state)
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
                    _focusedWidget = entity;

                    events.OnFocused();
                }

                if (events.IsClickable())
                {
                    events.OnClick();
                }
            }

            return true;
        }
    }

    return false;
}

void UIRenderer::OnMousePositionUpdate(Window* window, f32 x, f32 y)
{
    // TODO FEATURE: Handle Dragging
}

bool UIRenderer::OnKeyboardInput(Window* window, i32 key, i32 action, i32 modifiers)
{
    if (_focusedWidget == entt::null)
        return false;

    if (action != GLFW_RELEASE)
        return true;

    entt::registry* registry = ServiceLocator::GetUIRegistry();
    UITransformEvents& events = registry->get<UITransformEvents>(_focusedWidget);

    if (key == GLFW_KEY_ESCAPE)
    {
        events.OnUnfocused();
        _focusedWidget = entt::null;

        return true;
    }

    UITransform& transform = registry->get<UITransform>(_focusedWidget);
    if (transform.type == UIElementType::UITYPE_INPUTFIELD)
    {
        UI::asInputField* inputFieldAS = reinterpret_cast<UI::asInputField*>(transform.asObject);
        UIInputField& inputField = registry->get<UIInputField>(_focusedWidget);

        switch (key)
        {
        case GLFW_KEY_BACKSPACE:
            inputFieldAS->RemovePreviousCharacter();
            break;
        case GLFW_KEY_DELETE:
            inputFieldAS->RemoveNextCharacter();
            break;
        case GLFW_KEY_LEFT:
            inputFieldAS->MovePointerLeft();
            break;
        case GLFW_KEY_RIGHT:
            inputFieldAS->MovePointerRight();
            break;
        case GLFW_KEY_ENTER:
            inputField.OnSubmit();
            events.OnUnfocused();
            _focusedWidget = entt::null;
            break;
        default:
            break;
        }
    }

    return true;
}

bool UIRenderer::OnCharInput(Window* window, u32 unicodeKey)
{
    if (_focusedWidget != entt::null)
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UITransform& transform = registry->get<UITransform>(_focusedWidget);

        if (transform.type == UIElementType::UITYPE_INPUTFIELD)
        {
            UI::asInputField* inputField = reinterpret_cast<UI::asInputField*>(transform.asObject);

            inputField->AppendInput((char)unicodeKey);
        }

        return true;
    }

    return false;
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
}

Renderer::TextureID UIRenderer::ReloadTexture(const std::string& texturePath)
{
    Renderer::TextureDesc textureDesc;
    textureDesc.path = texturePath;

    return _renderer->LoadTexture(textureDesc);
}

void UIRenderer::CalculateVertices(const vec3& pos, const vec2& size, std::vector<Renderer::Vertex>& vertices)
{
    vec3 upperLeftPos = vec3(pos.x, pos.y, -pos.z);
    vec3 upperRightPos = vec3(pos.x + size.x, pos.y, -pos.z);
    vec3 lowerLeftPos = vec3(pos.x, pos.y + size.y, -pos.z);
    vec3 lowerRightPos = vec3(pos.x + size.x, pos.y + size.y, -pos.z);

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
