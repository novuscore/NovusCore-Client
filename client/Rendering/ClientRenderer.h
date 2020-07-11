#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/InstanceData.h>
#include <Renderer/DescriptorSet.h>

#include "ViewConstantBuffer.h"

namespace Renderer
{
    class Renderer;
}

namespace Memory
{
    class StackAllocator;
}

class Window;
class Camera;
class UIRenderer;
class TerrainRenderer;
class InputManager;

class ClientRenderer
{
public:
    ClientRenderer();

    bool UpdateWindow(f32 deltaTime);
    void Update(f32 deltaTime);
    void Render();

    u8 GetFrameIndex() { return _frameIndex; }
    UIRenderer* GetUIRenderer() { return _uiRenderer; }
private:
    void CreatePermanentResources();

private:
    Window* _window;
    Camera* _camera;
    InputManager* _inputManager;
    Renderer::Renderer* _renderer;
    Memory::StackAllocator* _frameAllocator;

    u8 _frameIndex = 0;

    // Permanent resources
    Renderer::ImageID _mainColor;

    Renderer::DepthImageID _mainDepth;

    Renderer::ModelID _cubeModel;
    Renderer::TextureID _cubeTexture;
    Renderer::InstanceData _cubeModelInstance;
    Renderer::SamplerID _linearSampler;

    Renderer::ConstantBuffer<ViewConstantBuffer>* _viewConstantBuffer;

    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _drawDescriptorSet;

    // Sub renderers
    UIRenderer* _uiRenderer;
    TerrainRenderer* _terrainRenderer;

    u8 _debugDrawingMode = 0;
    bool _isMinimized = false;
};