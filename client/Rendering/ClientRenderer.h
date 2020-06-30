#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/InstanceData.h>

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
    Renderer::ImageID _debugTextureID;
    Renderer::ImageID _debugAlphaMap;

    Renderer::DepthImageID _mainDepth;

    Renderer::ModelID _cubeModel;
    Renderer::TextureID _cubeTexture;
    Renderer::InstanceData _cubeModelInstance;
    Renderer::SamplerID _linearSampler;

    Renderer::ConstantBuffer<ViewConstantBuffer>* _viewConstantBuffer;

    // Sub renderers
    UIRenderer* _uiRenderer;
    TerrainRenderer* _terrainRenderer;

    u8 _debugDrawingMode = 0;
};