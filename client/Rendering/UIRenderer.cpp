#include "UIRenderer.h"
#include "Camera.h"
#include "UIElementRegistry.h"

#include "../Utils/ServiceLocator.h"
#include "../UI/Widget/Widget.h"
#include "../UI/Widget/Panel.h"
#include "../UI/Widget/Label.h"
#include "../UI/Widget/Button.h"
#include "../UI/Widget/InputField.h"

#include <Renderer/Renderer.h>
#include <Renderer/Renderers/Vulkan/RendererVK.h>
#include <Renderer/Descriptors/FontDesc.h>
#include <Window/Window.h>
#include <InputManager.h>
#include <GLFW/glfw3.h>

const int WIDTH = 1920;
const int HEIGHT = 1080;

UIRenderer::UIRenderer(Renderer::Renderer* renderer) : _focusedField(nullptr)
{
    _renderer = renderer;
    CreatePermanentResources();

    InputManager* inputManager = ServiceLocator::GetInputManager();
    inputManager->RegisterKeybind("UI Click Checker", GLFW_MOUSE_BUTTON_LEFT, KEYBIND_ACTION_CLICK, KEYBIND_MOD_ANY, std::bind(&UIRenderer::OnMouseClick, this, std::placeholders::_1, std::placeholders::_2));
    inputManager->RegisterMousePositionCallback("UI Mouse Position Checker", std::bind(&UIRenderer::OnMousePositionUpdate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    inputManager->RegisterKeyboardInputCallback("UI Keyboard Input Checker"_h, std::bind(&UIRenderer::OnKeyboardInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    inputManager->RegisterCharInputCallback("UI Char Input Cheker"_h, std::bind(&UIRenderer::OnCharInput, this, std::placeholders::_1, std::placeholders::_2));
}

void UIRenderer::Update(f32 deltaTime)
{
    UIElementRegistry* uiElementRegistry = UIElementRegistry::Instance();

    for (auto panel : uiElementRegistry->GetPanels()) // TODO: Store panels in a better manner than this
    {
        if (panel->IsDirty())
        {
            if (panel->GetTexture().length() == 0)
            {
                panel->ResetDirty();
                continue;
            }

            // (Re)load texture
            Renderer::TextureID textureID = ReloadTexture(panel->GetTexture());
            panel->SetTextureID(textureID);

            // Update position depending on parents etc
            // TODO

            Renderer::PrimitiveModelDesc primitiveModelDesc;

            // Update vertex buffer
            const vec3 pos = vec3(panel->GetScreenPosition(), 0);
            const vec2& size = panel->GetSize();

            CalculateVertices(pos, size, primitiveModelDesc.vertices);

            Renderer::ModelID modelID = panel->GetModelID();
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

                Renderer::ModelID modelID = _renderer->CreatePrimitiveModel(primitiveModelDesc);
                panel->SetModelID(modelID);
            }
            else // Otherwise we just update the already existing primitive model
            {
                _renderer->UpdatePrimitiveModel(modelID, primitiveModelDesc);
            }

            // Create constant buffer if necessary
            auto constantBuffer = panel->GetConstantBuffer();
            if (constantBuffer == nullptr)
            {
                constantBuffer = _renderer->CreateConstantBuffer<UI::Panel::PanelConstantBuffer>();
                panel->SetConstantBuffer(constantBuffer);
            }
            constantBuffer->resource.color = panel->GetColor();
            constantBuffer->Apply(0);
            constantBuffer->Apply(1);

            panel->ResetDirty();
        }
    }

    for (auto button : uiElementRegistry->GetButtons()) // TODO: Store panels in a better manner than this
    {
        if (button->IsDirty())
        {
            if (button->GetTexture().length() == 0)
            {
                button->ResetDirty();
                continue;
            }

            // (Re)load texture
            Renderer::TextureID textureID = ReloadTexture(button->GetTexture());
            button->SetTextureID(textureID);

            // Update position depending on parents etc
            // TODO

            Renderer::PrimitiveModelDesc primitiveModelDesc;

            // Update vertex buffer
            const vec3 pos = vec3(button->GetScreenPosition(), 0);
            const vec2& size = button->GetSize();

            CalculateVertices(pos, size, primitiveModelDesc.vertices);

            Renderer::ModelID modelID = button->GetModelID();
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

                Renderer::ModelID modelID = _renderer->CreatePrimitiveModel(primitiveModelDesc);
                button->SetModelID(modelID);
            }
            else // Otherwise we just update the already existing primitive model
            {
                _renderer->UpdatePrimitiveModel(modelID, primitiveModelDesc);
            }

            // Create constant buffer if necessary
            auto constantBuffer = button->GetConstantBuffer();
            if (constantBuffer == nullptr)
            {
                constantBuffer = _renderer->CreateConstantBuffer<UI::Button::ButtonConstantBuffer>();
                button->SetConstantBuffer(constantBuffer);
            }
            constantBuffer->resource.color = button->GetColor();
            constantBuffer->Apply(0);
            constantBuffer->Apply(1);

            button->ResetDirty();
        }
    }

    for (auto inputField : uiElementRegistry->GetInputFields()) // TODO: Store panels in a better manner than this
    {
        if (inputField->IsDirty())
        {
            if (inputField->GetTexture().length() == 0)
            {
                inputField->ResetDirty();
                continue;
            }

            // (Re)load texture
            Renderer::TextureID textureID = ReloadTexture(inputField->GetTexture());
            inputField->SetTextureID(textureID);

            // Update position depending on parents etc
            // TODO

            Renderer::PrimitiveModelDesc primitiveModelDesc;

            // Update vertex buffer
            const vec3 pos = vec3(inputField->GetScreenPosition(), 0);
            const vec2& size = inputField->GetSize();

            CalculateVertices(pos, size, primitiveModelDesc.vertices);

            Renderer::ModelID modelID = inputField->GetModelID();
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

                Renderer::ModelID modelID = _renderer->CreatePrimitiveModel(primitiveModelDesc);
                inputField->SetModelID(modelID);
            }
            else // Otherwise we just update the already existing primitive model
            {
                _renderer->UpdatePrimitiveModel(modelID, primitiveModelDesc);
            }

            // Create constant buffer if necessary
            auto constantBuffer = inputField->GetConstantBuffer();
            if (constantBuffer == nullptr)
            {
                constantBuffer = _renderer->CreateConstantBuffer<UI::InputField::InputFieldConstantBuffer>();
                inputField->SetConstantBuffer(constantBuffer);
            }
            constantBuffer->resource.color = inputField->GetColor();
            constantBuffer->Apply(0);
            constantBuffer->Apply(1);

            inputField->ResetDirty();
        }
    }

    for (auto label : uiElementRegistry->GetLabels()) // TODO: Store labels in a better manner than this
    {
        if (label->IsDirty())
        {
            // (Re)load font
            label->_font = Renderer::Font::GetFont(_renderer, label->GetFontPath(), label->GetFontSize());

            // Grow arrays if necessary
            const std::string& text = label->GetText();

            size_t textLength = text.size();
            size_t textLengthWithoutSpaces = std::count_if(text.begin(), text.end(), [](char c)
                {
                    return c != ' ';
                });

            size_t glyphCount = label->_models.size();

            if (label->_models.size() < textLengthWithoutSpaces)
            {
                label->_models.resize(textLengthWithoutSpaces);
                for (size_t i = glyphCount; i < textLengthWithoutSpaces; i++)
                {
                    label->_models[i] = Renderer::ModelID::Invalid();
                }
            }
            if (label->_textures.size() < textLengthWithoutSpaces)
            {
                label->_textures.resize(textLengthWithoutSpaces);
                for (size_t i = glyphCount; i < textLengthWithoutSpaces; i++)
                {
                    label->_textures[i] = Renderer::TextureID::Invalid();
                }
            }

            size_t glyph = 0;
            vec3 currentPosition = vec3(label->GetScreenPosition(), 0);
            for (size_t i = 0; i < textLength; i++)
            {
                char character = text[i];

                // If we encounter a space we just advance currentPosition and continue
                if (character == ' ')
                {
                    currentPosition.x += label->GetFontSize() * 0.15f;
                    continue;
                }

                Renderer::FontChar& fontChar = label->_font->GetChar(character);

                const vec3& pos = currentPosition + vec3(fontChar.xOffset, fontChar.yOffset, 0);
                const vec2& size = vec2(fontChar.width, fontChar.height);

                Renderer::PrimitiveModelDesc primitiveModelDesc;
                primitiveModelDesc.debugName = "Label " + character;

                CalculateVertices(pos, size, primitiveModelDesc.vertices);

                Renderer::ModelID modelID = label->_models[glyph];

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

                    label->_models[glyph] = _renderer->CreatePrimitiveModel(primitiveModelDesc);
                }
                else // Otherwise we just update the already existing primitive model
                {
                    _renderer->UpdatePrimitiveModel(modelID, primitiveModelDesc);
                }

                label->_textures[glyph] = fontChar.texture;

                currentPosition.x += fontChar.advance;
                glyph++;
            }

            // Create constant buffer if necessary
            auto constantBuffer = label->GetConstantBuffer();
            if (constantBuffer == nullptr)
            {
                constantBuffer = _renderer->CreateConstantBuffer<UI::Label::LabelConstantBuffer>();
                label->SetConstantBuffer(constantBuffer);
            }
            constantBuffer->resource.textColor = label->GetColor();
            constantBuffer->resource.outlineColor = label->GetOutlineColor();
            constantBuffer->resource.outlineWidth = label->GetOutlineWidth();
            constantBuffer->Apply(0);
            constantBuffer->Apply(1);

            label->ResetDirty();
        }
    }
}

void UIRenderer::AddUIPass(Renderer::RenderGraph* renderGraph, Renderer::ImageID renderTarget, u8 frameIndex)
{
    // UI Pass
    UIElementRegistry* uiElementRegistry = UIElementRegistry::Instance();

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
        [&, uiElementRegistry](UIPassData& data, Renderer::CommandList& commandList) // Execute
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
            for (auto panel : uiElementRegistry->GetPanels())
            {
                commandList.PushMarker("Panel", Color(0.0f, 0.1f, 0.0f));

                // Set constant buffer
                commandList.SetConstantBuffer(0, panel->GetConstantBuffer()->GetGPUResource(frameIndex));

                // Set texture-sampler pair
                commandList.SetTextureSampler(1, panel->GetTextureID(), _linearSampler);

                // Draw
                commandList.Draw(panel->GetModelID());

                commandList.PopMarker();
            }
            commandList.EndPipeline(pipeline);

            // Set pipeline
            pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            // Draw all the buttons
            for (auto button : uiElementRegistry->GetButtons())
            {
                if (button->GetTexture().length() == 0)
                    continue;

                commandList.PushMarker("Button", Color(0.0f, 0.0f, 0.0f));

                // Set constant buffer
                commandList.SetConstantBuffer(0, button->GetConstantBuffer()->GetGPUResource(frameIndex));

                // Set texture-sampler pair
                commandList.SetTextureSampler(1, button->GetTextureID(), _linearSampler);

                // Draw
                commandList.Draw(button->GetModelID());

                commandList.PopMarker();
            }
            commandList.EndPipeline(pipeline);

            // Set pipeline
            pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);
            
            // Draw all the inputFields
            for (auto inputField : uiElementRegistry->GetInputFields())
            {
                if (inputField->GetTexture().length() == 0)
                    continue;

                commandList.PushMarker("InputField", Color(0.0f, 0.0f, 0.0f));

                // Set constant buffer
                commandList.SetConstantBuffer(0, inputField->GetConstantBuffer()->GetGPUResource(frameIndex));

                // Set texture-sampler pair
                commandList.SetTextureSampler(1, inputField->GetTextureID(), _linearSampler);

                // Draw
                commandList.Draw(inputField->GetModelID());

                commandList.PopMarker();
            }
            commandList.EndPipeline(pipeline);

            // Draw text
            vertexShaderDesc.path = "Data/shaders/text.vert.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            pixelShaderDesc.path = "Data/shaders/text.frag.spv";
            pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

            // Set pipeline
            pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            // Draw all the labels
            for (auto label : uiElementRegistry->GetLabels())
            {
                commandList.PushMarker("Label", Color(0.0f, 0.1f, 0.0f));

                // Set constant buffer
                commandList.SetConstantBuffer(0, label->GetConstantBuffer()->GetGPUResource(frameIndex));

                // Each glyph in the label has it's own plane and texture, this could be optimized in the future.
                u32 glyphs = label->GetGlyphCount();
                for (u32 i = 0; i < glyphs; i++)
                {
                    // Set texture-sampler pair
                    commandList.SetTextureSampler(1, label->_textures[i], _linearSampler);

                    // Draw
                    commandList.Draw(label->_models[i]);
                }

                commandList.PopMarker();
            }
            commandList.EndPipeline(pipeline);
        });
}

bool UIRenderer::OnMouseClick(Window* window, std::shared_ptr<Keybind> keybind)
{
    UIElementRegistry* uiElementRegistry = UIElementRegistry::Instance();
    InputManager* inputManager = ServiceLocator::GetInputManager();
    f32 mouseX = inputManager->GetMousePositionX();
    f32 mouseY = inputManager->GetMousePositionY();

    for (auto panel : uiElementRegistry->GetPanels()) // TODO: Store panels in a better manner than this
    {
        if (!panel->IsClickable() && !panel->IsDraggable())
            continue;

        const vec2& size = panel->GetSize();
        const vec2& pos = panel->GetScreenPosition();

        if ((mouseX > pos.x && mouseX < pos.x + size.x) &&
            (mouseY > pos.y && mouseY < pos.y + size.y))
        {
            if (keybind->state)
            {
                if (panel->IsDraggable())
                    panel->BeingDrag(vec2(mouseX - pos.x, mouseY - pos.y));
            }
            else
            {
                if (panel->IsClickable())
                {
                    if (!panel->DidDrag())
                        panel->OnClick();
                }
            }

            return true;
        }

        if (!keybind->state)
        {
            if (panel->IsDraggable())
            {
                panel->EndDrag();

                return true;
            }
        }
    }

    for (auto button : uiElementRegistry->GetButtons()) // TODO: Store buttons in a better manner than this
    {
        if (!button->IsClickable())
            continue;

        const vec2& size = button->GetSize();
        const vec2& pos = button->GetScreenPosition();

        if ((mouseX > pos.x && mouseX < pos.x + size.x) &&
            (mouseY > pos.y && mouseY < pos.y + size.y))
        {
            if (keybind->state)
            {
                button->OnClick();

                return true;
            }
        }
    }

    //Focus/Unfocus field if we clicked.
    if (keybind->state)
    {
        //Unfocus existing field.
        if (_focusedField)
        {
            _focusedField->OnSubmit();
            _focusedField = nullptr;
        }

        //Focus new field.
        for (auto inputField : uiElementRegistry->GetInputFields())
        {
            const vec2& size = inputField->GetSize();
            const vec2& pos = inputField->GetScreenPosition();

            if ((mouseX > pos.x && mouseX < pos.x + size.x) &&
                (mouseY > pos.y && mouseY < pos.y + size.y))
            {
                _focusedField = inputField;

                return true;
            }
        }
    }

    return false;
}

void UIRenderer::OnMousePositionUpdate(Window* window, f32 x, f32 y)
{
    UIElementRegistry* uiElementRegistry = UIElementRegistry::Instance();

    for (auto panel : uiElementRegistry->GetPanels()) // TODO: Store panels in a better manner than this
    {
        if (!panel->IsDraggable())
            continue;

        if (!panel->IsDragging())
            continue;

        if (!panel->DidDrag())
            panel->SetDidDrag();

        const vec2& deltaDragPosition = panel->GetDeltaDragPosition();
        const vec2& size = panel->GetSize();
        vec2 newPosition(x - deltaDragPosition.x, y - deltaDragPosition.y);

        panel->SetPosition(newPosition, 0);
        panel->SetDirty();
    }
}

bool UIRenderer::OnKeyboardInput(Window* window, i32 key, i32 action, i32 modifiers)
{
    if (!_focusedField)
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

    return true;
}

bool UIRenderer::OnCharInput(Window* window, u32 unicodeKey)
{
    if (!_focusedField)
        return false;

    std::string input = "";
    input.append(1, (char)unicodeKey);
    _focusedField->AddText(input);

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
