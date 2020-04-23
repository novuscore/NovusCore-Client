#include "ClientRenderer.h"
#include "UIRenderer.h"
#include "Camera.h"
#include "../Utils/ServiceLocator.h"

#include <Renderer/Renderer.h>
#include <Renderer/Renderers/Vulkan/RendererVK.h>
#include <Window/Window.h>
#include <InputManager.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

const int WIDTH = 1920;
const int HEIGHT = 1080;
const size_t FRAME_ALLOCATOR_SIZE = 8 * 1024 * 1024; // 8 MB
u32 MAIN_RENDER_LAYER = "MainLayer"_h; // _h will compiletime hash the string into a u32

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 modifiers)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    ServiceLocator::GetInputManager()->KeyboardInputHandler(userWindow, key, scancode, action, modifiers);
}
void char_callback(GLFWwindow* window, u32 unicodeKey)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    ServiceLocator::GetInputManager()->CharInputHandler(userWindow, unicodeKey);
}
void mouse_callback(GLFWwindow* window, i32 button, i32 action, i32 modifiers)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    ServiceLocator::GetInputManager()->MouseInputHandler(userWindow, button, action, modifiers);
}
void cursor_position_callback(GLFWwindow* window, f64 x, f64 y)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    ServiceLocator::GetInputManager()->MousePositionHandler(userWindow, static_cast<f32>(x), static_cast<f32>(y));
}

ClientRenderer::ClientRenderer()
{
    _camera = new Camera(vec3(0, 0, -10));
    _window = new Window();
    _window->Init(WIDTH, HEIGHT);
    ServiceLocator::SetWindow(_window);

    _inputManager = new InputManager();
    ServiceLocator::SetInputManager(_inputManager);

    // We have to call Init here as we use the InputManager
    _camera->Init();

    glfwSetKeyCallback(_window->GetWindow(), key_callback);
    glfwSetCharCallback(_window->GetWindow(), char_callback);
    glfwSetMouseButtonCallback(_window->GetWindow(), mouse_callback);
    glfwSetCursorPosCallback(_window->GetWindow(), cursor_position_callback);


    _renderer = new Renderer::RendererVK();
    _renderer->InitWindow(_window);

    CreatePermanentResources();
    _uiRenderer = new UIRenderer(_renderer);
}

bool ClientRenderer::UpdateWindow(f32 deltaTime)
{
    return _window->Update(deltaTime);
}

void ClientRenderer::Update(f32 deltaTime)
{
    // Reset the memory in the frameAllocator
    _frameAllocator->Reset();

    // Update the camera movement
    _camera->Update(deltaTime);
    
    // Update the view matrix to match the new camera position
    
    _viewConstantBuffer->resource.viewMatrix = _camera->GetViewMatrix();
    _viewConstantBuffer->Apply(_frameIndex);

    // Register models to be rendered TODO: Push this to the ECS later
    Renderer::RenderLayer& mainLayer = _renderer->GetRenderLayer(MAIN_RENDER_LAYER);
    mainLayer.Reset(); // Reset the layer first so we don't just infinitely grow our layer
    mainLayer.RegisterModel(_cubeModel, &_cubeModelInstance);

    _uiRenderer->Update(deltaTime);
}

void ClientRenderer::Render()
{
    // Create rendergraph
    Renderer::RenderGraphDesc renderGraphDesc;
    renderGraphDesc.allocator = _frameAllocator; // We need to give our rendergraph an allocator to use
    Renderer::RenderGraph renderGraph = _renderer->CreateRenderGraph(renderGraphDesc);
    
    // Main Pass
    {
        struct MainPassData
        {
            Renderer::RenderPassMutableResource mainColor;
            Renderer::RenderPassResource cubeTexture;
        };

        renderGraph.AddPass<MainPassData>("Main Pass",
            [&](MainPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
            {
                data.mainColor = builder.Write(_mainColor, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
                data.cubeTexture = builder.Read(_cubeTexture, Renderer::RenderGraphBuilder::ShaderStage::SHADER_STAGE_PIXEL);

                return true; // Return true from setup to enable this pass, return false to disable it
            },
            [&](MainPassData& data, Renderer::CommandList& commandList) // Execute
            {
                Renderer::GraphicsPipelineDesc pipelineDesc;
                renderGraph.InitializePipelineDesc(pipelineDesc);

                // Shaders
                Renderer::VertexShaderDesc vertexShaderDesc;
                vertexShaderDesc.path = "Data/shaders/test.vert.spv";
                pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

                Renderer::PixelShaderDesc pixelShaderDesc;
                pixelShaderDesc.path = "Data/shaders/test.frag.spv";
                pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

                // Constant buffers  TODO: Improve on this, if I set state 0 and 3 it won't work etc...
                pipelineDesc.states.constantBufferStates[0].enabled = true; // ViewCB
                pipelineDesc.states.constantBufferStates[0].shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_VERTEX;
                pipelineDesc.states.constantBufferStates[1].enabled = true; // ModelCB
                pipelineDesc.states.constantBufferStates[1].shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_VERTEX;

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
                pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

                // Samplers TODO: We don't care which samplers we have here, we just need the number of samplers
                pipelineDesc.states.samplers[0].enabled = true;

                // Textures TODO: We don't care which textures we have here, we just need the number of textures
                pipelineDesc.textures[0] = data.cubeTexture;

                // Render targets
                pipelineDesc.renderTargets[0] = data.mainColor;

                // Clear mainColor TODO: This should be handled by the parameter in Setup, and it should definitely not act on ImageID and DepthImageID
                commandList.Clear(_mainColor, Color(0, 0, 0, 1));
                
                // Set pipeline
                Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
                commandList.BeginPipeline(pipeline);

                // Set view constant buffer
                commandList.SetConstantBuffer(0, _viewConstantBuffer->GetGPUResource(_frameIndex));

                // Set texture-sampler pair
                commandList.SetTextureSampler(2, _cubeTexture, _linearSampler);

                // Render main layer
                Renderer::RenderLayer& mainLayer = _renderer->GetRenderLayer(MAIN_RENDER_LAYER);

                for (auto const& model : mainLayer.GetModels())
                {
                    auto const& modelID = Renderer::ModelID(model.first);
                    auto const& instances = model.second;

                    for (auto const& instance : instances)
                    {
                        // Update model constant buffer
                        _modelConstantBuffer->resource.modelMatrix = instance->modelMatrix;
                        _modelConstantBuffer->resource.colorMultiplier = instance->colorMultiplier;
                        _modelConstantBuffer->Apply(_frameIndex);

                        // Set model constant buffer
                        commandList.SetConstantBuffer(1, _modelConstantBuffer->GetGPUResource(_frameIndex));

                        // Draw
                        commandList.Draw(modelID);
                    }
                }
                commandList.EndPipeline(pipeline);
            });
    }

    _uiRenderer->AddUIPass(&renderGraph, _mainColor, _frameIndex);

    renderGraph.Setup();
    renderGraph.Execute();
    
    _renderer->Present(_window, _mainColor);

    // Flip the frameIndex between 0 and 1
    _frameIndex = !_frameIndex;
}

void ClientRenderer::CreatePermanentResources()
{
    // Main color rendertarget
    Renderer::ImageDesc mainColorDesc;
    mainColorDesc.debugName = "MainColor";
    mainColorDesc.dimensions = ivec2(WIDTH, HEIGHT);
    mainColorDesc.format = Renderer::IMAGE_FORMAT_R16G16B16A16_FLOAT;
    mainColorDesc.sampleCount = Renderer::SAMPLE_COUNT_1;


    _mainColor = _renderer->CreateImage(mainColorDesc);

    // Main depth rendertarget
    Renderer::DepthImageDesc mainDepthDesc;
    mainDepthDesc.debugName = "MainDepth";
    mainDepthDesc.dimensions = ivec2(WIDTH, HEIGHT);
    mainDepthDesc.format = Renderer::DEPTH_IMAGE_FORMAT_D32_FLOAT;
    mainDepthDesc.sampleCount = Renderer::SAMPLE_COUNT_1;

    _mainDepth = _renderer->CreateDepthImage(mainDepthDesc);

    // Cube model TODO: This is unnecessary once we have some kind of Scene abstraction
    Renderer::ModelDesc modelDesc;
    modelDesc.path = "Data/models/Cube.novusmodel";

    _cubeModel = _renderer->LoadModel(modelDesc);

    Renderer::TextureDesc textureDesc;
    textureDesc.path = "Data/textures/debug.jpg";
    
    _cubeTexture = _renderer->LoadTexture(textureDesc);

    // Sampler
    Renderer::SamplerDesc samplerDesc;
    samplerDesc.enabled = true;
    samplerDesc.filter = Renderer::SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.addressU = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressV = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.addressW = Renderer::TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.shaderVisibility = Renderer::ShaderVisibility::SHADER_VISIBILITY_PIXEL;

    _linearSampler = _renderer->CreateSampler(samplerDesc);

    // View Constant Buffer (for camera data)
    _viewConstantBuffer = _renderer->CreateConstantBuffer<ViewConstantBuffer>();
    {
        mat4x4& projMatrix = _viewConstantBuffer->resource.projMatrix;

        const f32 fov = 68.0f;
        const f32 nearClip = 0.1f;
        const f32 farClip = 100.0f;
        f32 aspectRatio = static_cast<f32>(WIDTH) / static_cast<f32>(HEIGHT);

        projMatrix = glm::perspective(fov, aspectRatio, nearClip, farClip);

        _viewConstantBuffer->Apply(0);
        _viewConstantBuffer->Apply(1);
    }

    // Model Constant Buffer (for per-model data)
    _modelConstantBuffer = _renderer->CreateConstantBuffer<ModelConstantBuffer>();

    // Frame allocator, this is a fast allocator for data that is only needed this frame
    _frameAllocator = new Memory::StackAllocator(FRAME_ALLOCATOR_SIZE);
    _frameAllocator->Init();
}
