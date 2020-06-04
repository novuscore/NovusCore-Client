#include "UIRenderer.h"
#include "Camera.h"

#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/UI/UIEntityPoolSingleton.h"
#include "../ECS/Components/UI/UIAddElementQueueSingleton.h"
#include "../ECS/Components/UI/UIDataSingleton.h"
#include "../ECS/Components/UI/UITransform.h"
#include "../ECS/Components/UI/UITransformUtils.h"
#include "../ECS/Components/UI/UITransformEvents.h"
#include "../ECS/Components/UI/UIRenderable.h"
#include "../ECS/Components/UI/UIText.h"

#include <Renderer/Renderer.h>
#include <Renderer/Renderers/Vulkan/RendererVK.h>
#include <Renderer/Descriptors/FontDesc.h>
#include <Window/Window.h>
#include <InputManager.h>
#include <GLFW/glfw3.h>

const int WIDTH = 1920;
const int HEIGHT = 1080;

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

    // Preallocate Entity Ids
    UIEntityPoolSingleton& entityPool = registry->set<UIEntityPoolSingleton>();
    UIAddElementQueueSingleton& addElementQueue = registry->set<UIAddElementQueueSingleton>();
    std::vector<entt::entity> entityIds(10000);
    registry->create(entityIds.begin(), entityIds.end());
    entityPool.entityIdPool.enqueue_bulk(entityIds.begin(), 10000);

    // Register UI data singleton.
    registry->set<UI::UIDataSingleton>();
}

void UIRenderer::Update(f32 deltaTime)
{
    entt::registry* registry = ServiceLocator::GetUIRegistry();
    auto renderableView = registry->view<UITransform, UIRenderable>();
    renderableView.each([this](const auto, UITransform& transform, UIRenderable& renderable)
        {
            if (!transform.isDirty && !renderable.isDirty)
                return;

            if (renderable.isDirty)
            {
                if (renderable.texture.length() == 0)
                {
                    renderable.isDirty = false;
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

                renderable.isDirty = false;
            }

            if (transform.isDirty)
            {
                Renderer::PrimitiveModelDesc primitiveModelDesc;

                // Update vertex buffer
                const vec3& pos = vec3(UITransformUtils::GetMinBounds(transform), transform.depth);
                const vec2& size = transform.size;

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

                transform.isDirty = false;
            }
        });

    auto textView = registry->view<UITransform, UIText>();
    textView.each([this](const auto, UITransform& transform, UIText& text)
        {
            if (!transform.isDirty && !text.isDirty)
                return;

            if (text.fontPath.length() == 0)
            {
                transform.isDirty = false;
                return;
            }

            text.font = Renderer::Font::GetFont(_renderer, text.fontPath, text.fontSize);

            size_t textLengthWithoutSpaces = std::count_if(text.text.begin(), text.text.end(), [](char c)
                {
                    return c != ' ';
                });

            size_t glyphCount = text.models.size();
            if (glyphCount < textLengthWithoutSpaces)
            {
                text.models.resize(textLengthWithoutSpaces);
                for (size_t i = glyphCount; i < textLengthWithoutSpaces; i++)
                {
                    text.models[i] = Renderer::ModelID::Invalid();
                }
            }
            if (text.textures.size() < textLengthWithoutSpaces)
            {
                text.textures.resize(textLengthWithoutSpaces);                
                for (size_t i = glyphCount; i < textLengthWithoutSpaces; i++)
                {
                    text.textures[i] = Renderer::TextureID::Invalid();
                }
            }

            size_t glyph = 0;
            vec3 currentPosition = vec3(UITransformUtils::GetMinBounds(transform), transform.depth);
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

            transform.isDirty = false;
            text.isDirty = false;
        });
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
            auto renderableView = registry->view<UITransform, UIRenderable>();
            renderableView.each([this, &commandList, frameIndex](const auto, UITransform& _transform, UIRenderable& _renderable)
                {
                    if (!_renderable.constantBuffer)
                        return;

                    commandList.PushMarker("Renderable", Color(0.0f, 0.1f, 0.0f));

                    // Set constant buffer
                    commandList.SetConstantBuffer(0, _renderable.constantBuffer->GetGPUResource(frameIndex), frameIndex);

                    // Set Texture and sampler
                    commandList.SetTexture(1, _renderable.textureID);
                    commandList.SetSampler(2, _linearSampler);

                    // Draw
                    commandList.Draw(_renderable.modelID);

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

            auto textView = registry->view<UITransform, UIText>();
            textView.each([this, &commandList, frameIndex](const auto, UITransform& _transform, UIText& _text)
                {
                    if (!_text.constantBuffer)
                        return;

                    commandList.PushMarker("Text", Color(0.0f, 0.1f, 0.0f));

                    // Set constant buffer
                    commandList.SetConstantBuffer(0, _text.constantBuffer->GetGPUResource(frameIndex), frameIndex);

                    // Set sampler
                    commandList.SetSampler(1, _linearSampler);

                    // Each glyph in the label has it's own plane and texture, this could be optimized in the future.
                    size_t glyphs = _text.models.size();
                    for (u32 i = 0; i < glyphs; i++)
                    {
                        // Set texture
                        commandList.SetTexture(2, _text.textures[i]);

                        // Draw
                        commandList.Draw(_text.models[i]);
                    }

                    commandList.PopMarker();
                });

            commandList.EndPipeline(pipeline);
        });
}

bool UIRenderer::OnMouseClick(Window* window, std::shared_ptr<Keybind> keybind)
{
    InputManager* inputManager = ServiceLocator::GetInputManager();
    f32 mouseX = inputManager->GetMousePositionX();
    f32 mouseY = inputManager->GetMousePositionY();

    entt::registry* registry = ServiceLocator::GetUIRegistry();
    auto eventView = registry->view<UITransform, UITransformEvents>();
    for (auto entity : eventView)
    {
        auto& events = eventView.get<UITransformEvents>(entity);
        if (!events.flags)
            continue;

        auto& transform = eventView.get<UITransform>(entity);
        const vec2 minBounds = UITransformUtils::GetMinBounds(transform);
        const vec2 maxBounds = UITransformUtils::GetMaxBounds(transform);

        if ((mouseX > minBounds.x && mouseX < maxBounds.x) &&
            (mouseY > minBounds.y && mouseY < maxBounds.y))
        {
            if (keybind->state)
            {
                if (events.IsDraggable())
                {
                    //panel->BeingDrag(vec2(mouseX - pos.x, mouseY - pos.y));
                }
            }
            else
            {
                if (events.IsClickable())
                {
                    //if (!panel->DidDrag())

                    events.OnClick();
                }
            }

            return true;
        }

        if (!keybind->state)
        {
            if (events.IsDraggable())
            {
                //panel->EndDrag();

                return true;
            }
        }
    }

    return false;
}

void UIRenderer::OnMousePositionUpdate(Window* window, f32 x, f32 y)
{ 
}

bool UIRenderer::OnKeyboardInput(Window* window, i32 key, i32 action, i32 modifiers)
{
    /*if (!_focusedField)
        return false;

    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            _focusedField->OnSubmit();
            _focusedField = nullptr;
            break;

        case GLFW_KEY_ENTER:
            _focusedField->OnEnter();
            _focusedField = nullptr;
            break;

        case GLFW_KEY_BACKSPACE:
            _focusedField->RemovePreviousCharacter();
            break;

        case GLFW_KEY_DELETE:
            _focusedField->RemoveNextCharacter();
            break;

        case GLFW_KEY_LEFT:
            _focusedField->MovePointerLeft();
            break;

        case GLFW_KEY_RIGHT:
            _focusedField->MovePointerRight();
            break;

        default:
            break;
        }
    }
    */
    return false;
}

bool UIRenderer::OnCharInput(Window* window, u32 unicodeKey)
{
    /*if (!_focusedField)
        return false;

    std::string input = "";
    input.append(1, (char)unicodeKey);
    _focusedField->AddText(input);*/

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
    vec3 upperLeftPos = vec3(pos.x, pos.y, 0.0f);
    vec3 upperRightPos = vec3(pos.x + size.x, pos.y, 0.0f);
    vec3 lowerLeftPos = vec3(pos.x, pos.y + size.y, 0.0f);
    vec3 lowerRightPos = vec3(pos.x + size.x, pos.y + size.y, 0.0f);

    // UV space
    // TODO: Do scaling depending on rendertargets actual size instead of assuming 1080p (which is our reference resolution)
    upperLeftPos /= vec3(1920, 1080, 1.0f);
    upperRightPos /= vec3(1920, 1080, 1.0f);
    lowerLeftPos /= vec3(1920, 1080, 1.0f);
    lowerRightPos /= vec3(1920, 1080, 1.0f);

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
