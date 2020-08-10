#include "ClientRenderer.h"
#include "UIRenderer.h"
#include "TerrainRenderer.h"
#include "DebugRenderer.h"
#include "Camera.h"
#include "../Utils/ServiceLocator.h"

#include <Renderer/Renderer.h>
#include <Renderer/Renderers/Vulkan/RendererVK.h>
#include <Window/Window.h>
#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include <glm/gtc/matrix_transform.hpp>

const int WIDTH = 1920;
const int HEIGHT = 1080;
const size_t FRAME_ALLOCATOR_SIZE = 8 * 1024 * 1024; // 8 MB
u32 MAIN_RENDER_LAYER = "MainLayer"_h; // _h will compiletime hash the string into a u32
u32 DEPTH_PREPASS_RENDER_LAYER = "DepthPrepass"_h; // _h will compiletime hash the string into a u32

void KeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 modifiers)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    ServiceLocator::GetInputManager()->KeyboardInputHandler(userWindow, key, scancode, action, modifiers);
}

void CharCallback(GLFWwindow* window, u32 unicodeKey)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    ServiceLocator::GetInputManager()->CharInputHandler(userWindow, unicodeKey);
}

void MouseCallback(GLFWwindow* window, i32 button, i32 action, i32 modifiers)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    ServiceLocator::GetInputManager()->MouseInputHandler(userWindow, button, action, modifiers);
}

void CursorPositionCallback(GLFWwindow* window, f64 x, f64 y)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    ServiceLocator::GetInputManager()->MousePositionHandler(userWindow, static_cast<f32>(x), static_cast<f32>(y));
}

void WindowIconifyCallback(GLFWwindow* window, int iconified)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    userWindow->SetIsMinimized(iconified == 1);
}

ClientRenderer::ClientRenderer()
{
   _camera = new Camera(vec3(-8000.0f, 50.0f, 1600.0f)); // Goldshire
    //_camera = new Camera(vec3(300.0f, 0.0f, -4700.0f)); // Razor Hill
    //_camera = new Camera(vec3(3308.0f, 0.0f, 5316.0f)); // Borean Tundra

    //_camera = new Camera(vec3(0.0f, 0.0f, 0.0f));

    _window = new Window();
    _window->Init(WIDTH, HEIGHT);
    ServiceLocator::SetWindow(_window);

    _inputManager = new InputManager();
    ServiceLocator::SetInputManager(_inputManager);

    // We have to call Init here as we use the InputManager
    _camera->Init();

    glfwSetKeyCallback(_window->GetWindow(), KeyCallback);
    glfwSetCharCallback(_window->GetWindow(), CharCallback);
    glfwSetMouseButtonCallback(_window->GetWindow(), MouseCallback);
    glfwSetCursorPosCallback(_window->GetWindow(), CursorPositionCallback);
    glfwSetWindowIconifyCallback(_window->GetWindow(), WindowIconifyCallback);

    Renderer::TextureDesc debugTexture;
    debugTexture.path = "Data/textures/DebugTexture.bmp";
    
    _renderer = new Renderer::RendererVK(debugTexture);
    _renderer->InitWindow(_window);
    ServiceLocator::SetRenderer(_renderer);

    CreatePermanentResources();

    _debugRenderer = new DebugRenderer(_renderer);
    _uiRenderer = new UIRenderer(_renderer);
    _terrainRenderer = new TerrainRenderer(_renderer, _debugRenderer);

    _inputManager->RegisterKeybind("ToggleDebugDraw", GLFW_KEY_F1, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        _debugDrawingMode++;
        if (_debugDrawingMode > 2)
        {
            _debugDrawingMode = 0;
        }
        return true;
    });
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
    _camera->Update(deltaTime, 75.0f, (float)WIDTH / (float)HEIGHT);

    _terrainRenderer->Update(deltaTime, *_camera);

    _debugRenderer->DrawLine3D(vec3(0.0f, 0.0f, 0.0f), vec3(100.0f, 0.0f, 0.0f), 0xff0000ff);
    _debugRenderer->DrawLine3D(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 100.0f, 0.0f), 0xff00ff00);
    _debugRenderer->DrawLine3D(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 100.0f), 0xffff0000);
}

void ClientRenderer::Render()
{
    ZoneScopedNC("ClientRenderer::Render", tracy::Color::Red2)

    // If the window is minimized we want to pause rendering
    if (_window->IsMinimized())
        return;

    // Create rendergraph
    Renderer::RenderGraphDesc renderGraphDesc;
    renderGraphDesc.allocator = _frameAllocator; // We need to give our rendergraph an allocator to use
    Renderer::RenderGraph renderGraph = _renderer->CreateRenderGraph(renderGraphDesc);

    _renderer->FlipFrame(_frameIndex);

    // Update the view matrix to match the new camera position
    _viewConstantBuffer->resource.viewProjectionMatrix = _camera->GetViewProjectionMatrix();
    _viewConstantBuffer->Apply(_frameIndex);

    _passDescriptorSet.Bind("_viewData"_h, _viewConstantBuffer->GetBuffer(_frameIndex));

    // Depth Prepass
    {
        struct DepthPrepassData
        {
            Renderer::RenderPassMutableResource mainDepth;
        };

        renderGraph.AddPass<DepthPrepassData>("DepthPrepass",
            [=](DepthPrepassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.mainDepth = builder.Write(_mainDepth, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);

            return true;
        },
            [&](DepthPrepassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
        {
            TracySourceLocation(depthPrepass, "DepthPrepass", tracy::Color::Yellow2);
            commandList.BeginTrace(&depthPrepass);

            commandList.MarkFrameStart(_frameIndex);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            resources.InitializePipelineDesc(pipelineDesc);

            // Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/depthprepass.vs.hlsl.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

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

            // Depth state
            pipelineDesc.states.depthStencilState.depthEnable = true;
            pipelineDesc.states.depthStencilState.depthWriteEnable = true;
            pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_LESS;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;
            pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

            // Render targets
            pipelineDesc.depthStencil = data.mainDepth;

            // Clear mainColor TODO: This should be handled by the parameter in Setup, and it should definitely not act on ImageID and DepthImageID
            commandList.Clear(_mainDepth, 1.0f);

            // Set pipeline
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            commandList.SetViewport(0, 0, static_cast<f32>(WIDTH), static_cast<f32>(HEIGHT), 0.0f, 1.0f);
            commandList.SetScissorRect(0, WIDTH, 0, HEIGHT);

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, _frameIndex);

            // Render depth prepass layer
            //Renderer::RenderLayer& layer = _renderer->GetRenderLayer(DEPTH_PREPASS_RENDER_LAYER);
            //
            //for (auto const& model : layer.GetModels())
            //{
            //    auto const& modelID = Renderer::ModelID(model.first);
            //    auto const& instances = model.second;
            //
            //
            //    for (auto const& instance : instances)
            //    {
            //        instance->Apply(_frameIndex);
            //
            //        //_drawDescriptorSet.Bind("_modelData", instance->GetConstantBuffer());
            //        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawDescriptorSet, _frameIndex);
            //
            //        // Draw
            //        commandList.Draw(modelID);
            //    }
            //}
            commandList.EndPipeline(pipeline);
            commandList.EndTrace();
        });
    }

    // Terrain depth prepass
    _terrainRenderer->AddTerrainDepthPrepass(&renderGraph, _viewConstantBuffer, _mainDepth, _frameIndex);

    // Main Pass
    {
        struct MainPassData
        {
            Renderer::RenderPassMutableResource mainColor;
            Renderer::RenderPassMutableResource mainDepth;
            Renderer::RenderPassResource cubeTexture;
        };

        renderGraph.AddPass<MainPassData>("MainPass",
            [=](MainPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
        {
            data.mainColor = builder.Write(_mainColor, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
            data.mainDepth = builder.Write(_mainDepth, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_CLEAR);
            data.cubeTexture = builder.Read(_cubeTexture, Renderer::RenderGraphBuilder::ShaderStage::SHADER_STAGE_PIXEL);

            return true; // Return true from setup to enable this pass, return false to disable it
        },
            [&](MainPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
        {
            TracySourceLocation(mainPass, "MainPass", tracy::Color::Yellow2);
            commandList.BeginTrace(&mainPass);

            Renderer::GraphicsPipelineDesc pipelineDesc;
            resources.InitializePipelineDesc(pipelineDesc);

            // Shaders
            Renderer::VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/test.vs.hlsl.spv";
            pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);

            Renderer::PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "Data/shaders/test.ps.hlsl.spv";
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

            // Depth state
            pipelineDesc.states.depthStencilState.depthEnable = true;
            pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_EQUAL;

            // Rasterizer state
            pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;
            pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

            // Render targets
            pipelineDesc.renderTargets[0] = data.mainColor;

            pipelineDesc.depthStencil = data.mainDepth;

            // Clear mainColor TODO: This should be handled by the parameter in Setup, and it should definitely not act on ImageID and DepthImageID
            commandList.Clear(_mainColor, Color(0, 0, 0, 1));

            // Set pipeline
            Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
            commandList.BeginPipeline(pipeline);

            _drawDescriptorSet.Bind("_texture", _cubeTexture); // TODO: Actually select textures etc per draw

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_passDescriptorSet, _frameIndex);

            // Render main layer
            //Renderer::RenderLayer& mainLayer = _renderer->GetRenderLayer(MAIN_RENDER_LAYER);
            //
            //for (auto const& model : mainLayer.GetModels())
            //{
            //    auto const& modelID = Renderer::ModelID(model.first);
            //    auto const& instances = model.second;
            //
            //    for (auto const& instance : instances)
            //    {
            //        _drawDescriptorSet.Bind("_modelData", instance->GetBuffer(_frameIndex));
            //        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_drawDescriptorSet, _frameIndex);
            //
            //        // Draw
            //        commandList.Draw(modelID);
            //    }
            //}
            commandList.EndPipeline(pipeline);
            commandList.EndTrace();
        });
    }

    _terrainRenderer->AddTerrainPass(&renderGraph, _viewConstantBuffer, _mainColor, _mainDepth, _frameIndex, _debugDrawingMode, *_camera);
    
    _debugRenderer->Add3DPass(&renderGraph, _viewConstantBuffer->GetBuffer(_frameIndex), _mainColor, _mainDepth, _frameIndex);

    _uiRenderer->AddUIPass(&renderGraph, _mainColor, _frameIndex);

    renderGraph.AddSignalSemaphore(_sceneRenderedSemaphore); // Signal that we are ready to present
    renderGraph.AddSignalSemaphore(_frameSyncSemaphores.Get(_frameIndex)); // Signal that this frame has finished, for next frames sake

    static bool firstFrame = true;
    if (firstFrame)
    {
        firstFrame = false;
    }
    else
    {
        renderGraph.AddWaitSemaphore(_frameSyncSemaphores.Get(!_frameIndex)); // Wait for previous frame to finish
    }

    renderGraph.Setup();
    renderGraph.Execute();
    
    {
        ZoneScopedNC("Present", tracy::Color::Red2)
        _renderer->Present(_window, _mainColor, _sceneRenderedSemaphore); // Wait for the frame to render
    }

    // Flip the frameIndex between 0 and 1
    _frameIndex = !_frameIndex;
}

void ClientRenderer::CreatePermanentResources()
{
    // Main color rendertarget
    Renderer::ImageDesc mainColorDesc;
    mainColorDesc.debugName = "MainColor";
    mainColorDesc.dimensions = vec2(1.0f, 1.0f);
    mainColorDesc.dimensionType = Renderer::ImageDimensionType::DIMENSION_SCALE;
    mainColorDesc.format = Renderer::IMAGE_FORMAT_R16G16B16A16_FLOAT;
    mainColorDesc.sampleCount = Renderer::SAMPLE_COUNT_1;

    _mainColor = _renderer->CreateImage(mainColorDesc);

    // Main depth rendertarget
    Renderer::DepthImageDesc mainDepthDesc;
    mainDepthDesc.debugName = "MainDepth";
    mainDepthDesc.dimensions = vec2(1.0f, 1.0f);
    mainDepthDesc.dimensionType = Renderer::ImageDimensionType::DIMENSION_SCALE;
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
    _viewConstantBuffer = new Renderer::Buffer<ViewConstantBuffer>(_renderer, "ViewConstantBuffer", Renderer::BUFFER_USAGE_UNIFORM_BUFFER, Renderer::BufferCPUAccess::WriteOnly);

    // Create descriptor sets
    _passDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());
    _passDescriptorSet.Bind("_sampler"_h, _linearSampler);

    _drawDescriptorSet.SetBackend(_renderer->CreateDescriptorSetBackend());

    // Frame allocator, this is a fast allocator for data that is only needed this frame
    _frameAllocator = new Memory::StackAllocator(FRAME_ALLOCATOR_SIZE);
    _frameAllocator->Init();

    _sceneRenderedSemaphore = _renderer->CreateGPUSemaphore();
    for (u32 i = 0; i < _frameSyncSemaphores.Num; i++)
    {
        _frameSyncSemaphores.Get(i) = _renderer->CreateGPUSemaphore();
    }
}
