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

class CameraFreeLook;
class DebugRenderer;
class MapObjectRenderer;
struct RenderResources;

class SkyboxRenderer
{
public:
    SkyboxRenderer(Renderer::Renderer* renderer, DebugRenderer* debugRenderer);
    ~SkyboxRenderer();

    void Update(f32 deltaTime, const CameraFreeLook& camera);

    void AddSkyboxPass(Renderer::RenderGraph* renderGraph, RenderResources& resources, u8 frameIndex);

private:
    void CreatePermanentResources();

    Renderer::Renderer* _renderer;
    

    DebugRenderer* _debugRenderer;
};