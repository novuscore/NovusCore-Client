#pragma once
#include <NovusTypes.h>

#include <array>

#include <Utils/StringUtils.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/TextureArrayDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/Descriptors/BufferDesc.h>
#include <Renderer/Buffer.h>
#include <Renderer/DescriptorSet.h>

#include "../Gameplay/Map/Chunk.h"
#include "ViewConstantBuffer.h"

namespace Renderer
{
    class RenderGraph;
    class Renderer;
    class DescriptorSet;
}

class Camera;
class DebugRenderer;
class MapObjectRenderer;

class SkyboxRenderer
{
public:
    SkyboxRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer);
    ~SkyboxRenderer();

    void Update(f32 deltaTime, const Camera& camera);

    void AddSkyboxPass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex, const Camera& camera);

private:
    void CreatePermanentResources();

    Renderer::Renderer* _renderer;
    

    DebugRenderer* _debugRenderer;
};