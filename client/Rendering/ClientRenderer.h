#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Descriptors/GPUSemaphoreDesc.h>
#include <Renderer/DescriptorSet.h>
#include <Renderer/FrameResource.h>
#include <Renderer/Buffer.h>

#include "ViewConstantBuffer.h"
#include "LightConstantBuffer.h"

namespace Renderer
{
    class Renderer;
}

namespace Memory
{
    class StackAllocator;
}

class Window;
class CameraFreeLook;
class UIRenderer;
class TerrainRenderer;
class CModelRenderer;
class InputManager;
class DebugRenderer;
class PixelQuery;

class ClientRenderer
{
public:
    ClientRenderer();

    bool UpdateWindow(f32 deltaTime);
    void Update(f32 deltaTime);
    void Render();

    u8 GetFrameIndex() { return _frameIndex; }

    void InitImgui();
    UIRenderer* GetUIRenderer() { return _uiRenderer; }
    TerrainRenderer* GetTerrainRenderer() { return _terrainRenderer; }
    CModelRenderer* GetCModelRenderer() { return _cModelRenderer; }
    DebugRenderer* GetDebugRenderer() { return _debugRenderer; }
    PixelQuery* GetPixelQuery() { return _pixelQuery; }

    const std::string& GetGPUName();

    size_t GetVRAMUsage();
    size_t GetVRAMBudget();

    const i32 WIDTH = 1920;
    const i32 HEIGHT = 1080;
private:
    void CreatePermanentResources();

private:
    Window* _window;
    InputManager* _inputManager;
    Renderer::Renderer* _renderer;
    Memory::StackAllocator* _frameAllocator;

    u8 _frameIndex = 0;

    // Permanent resources
    Renderer::ImageID _mainColor;
    Renderer::ImageID _objectIDs;
    Renderer::ImageID _depthPyramid;

    Renderer::DepthImageID _mainDepth;

    Renderer::ModelID _cubeModel;
    Renderer::TextureID _cubeTexture;
    Renderer::SamplerID _linearSampler;

    Renderer::GPUSemaphoreID _sceneRenderedSemaphore; // This semaphore tells the present function when the scene is ready to be blitted and presented
    FrameResource<Renderer::GPUSemaphoreID, 2> _frameSyncSemaphores; // This semaphore makes sure the GPU handles frames in order

    Renderer::Buffer<ViewConstantBuffer>* _viewConstantBuffer;
    Renderer::Buffer<LightConstantBuffer>* _lightConstantBuffer;

    Renderer::DescriptorSet _globalDescriptorSet;
    Renderer::DescriptorSet _passDescriptorSet;
    Renderer::DescriptorSet _drawDescriptorSet;

    // Sub renderers
    DebugRenderer* _debugRenderer;
    UIRenderer* _uiRenderer;
    TerrainRenderer* _terrainRenderer;
    CModelRenderer* _cModelRenderer;

    PixelQuery* _pixelQuery;

    bool _isMinimized = false;
};