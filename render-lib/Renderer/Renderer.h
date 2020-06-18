#pragma once
#include <NovusTypes.h>
#include <Utils/StringUtils.h>
#include <robin_hood.h>
#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include "RenderLayer.h"
#include "RenderPass.h"
#include "ConstantBuffer.h"
#include "StorageBuffer.h"
#include "RenderStates.h"
#include "Font.h"

// Descriptors
#include "Descriptors/CommandListDesc.h"
#include "Descriptors/RendererDesc.h"
#include "Descriptors/VertexShaderDesc.h"
#include "Descriptors/PixelShaderDesc.h"
#include "Descriptors/ComputeShaderDesc.h"
#include "Descriptors/ImageDesc.h"
#include "Descriptors/TextureDesc.h"
#include "Descriptors/TextureArrayDesc.h"
#include "Descriptors/DepthImageDesc.h"
#include "Descriptors/ModelDesc.h"
#include "Descriptors/SamplerDesc.h"
#include "Descriptors/FontDesc.h"

class Window;

namespace Renderer
{
    class Renderer
    {
    public:
        virtual void InitWindow(Window* window) = 0;
        virtual void Deinit() = 0;

        virtual ~Renderer();

        RenderGraph CreateRenderGraph(RenderGraphDesc& desc);
        RenderLayer& GetRenderLayer(u32 layerHash);

        // Creation
        virtual ImageID CreateImage(ImageDesc& desc) = 0;
        virtual DepthImageID CreateDepthImage(DepthImageDesc& desc) = 0;

        virtual SamplerID CreateSampler(SamplerDesc& sampler) = 0;

        virtual GraphicsPipelineID CreatePipeline(GraphicsPipelineDesc& desc) = 0;
        virtual ComputePipelineID CreatePipeline(ComputePipelineDesc& desc) = 0;

        template <typename T>
        ConstantBuffer<T>* CreateConstantBuffer()
        {
            ConstantBuffer<T>* buffer = new ConstantBuffer<T>();
            buffer->backend = CreateBufferBackend(buffer->GetSize(), Backend::BufferBackend::TYPE_CONSTANT_BUFFER);

            return buffer;
        }

        template <typename T>
        StorageBuffer<T>* CreateStorageBuffer()
        {
            StorageBuffer<T>* buffer = new StorageBuffer<T>();
            buffer->backend = CreateBufferBackend(buffer->GetSize(), Backend::BufferBackend::TYPE_STORAGE_BUFFER);

            return buffer;
        }

        virtual ModelID CreatePrimitiveModel(PrimitiveModelDesc& desc) = 0;
        virtual void UpdatePrimitiveModel(ModelID model, PrimitiveModelDesc& desc) = 0;

        virtual TextureArrayID CreateTextureArray(TextureArrayDesc& desc) = 0;

        virtual TextureID CreateDataTexture(DataTextureDesc& desc) = 0;
        virtual TextureID CreateDataTextureIntoArray(DataTextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) = 0;
        
        // Loading
        virtual ModelID LoadModel(ModelDesc& desc) = 0;

        virtual TextureID LoadTexture(TextureDesc& desc) = 0;
        virtual TextureID LoadTextureIntoArray(TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) = 0;

        virtual VertexShaderID LoadShader(VertexShaderDesc& desc) = 0;
        virtual PixelShaderID LoadShader(PixelShaderDesc& desc) = 0;
        virtual ComputeShaderID LoadShader(ComputeShaderDesc& desc) = 0;

        // Command List Functions
        virtual CommandListID BeginCommandList() = 0;
        virtual void EndCommandList(CommandListID commandList) = 0;
        virtual void Clear(CommandListID commandList, ImageID image, Color color) = 0;
        virtual void Clear(CommandListID commandList, DepthImageID image, DepthClearFlags clearFlags, f32 depth, u8 stencil) = 0;
        virtual void Draw(CommandListID commandList, ModelID modelID) = 0;
        virtual void DrawBindless(CommandListID commandList, u32 numVertices, u32 numInstances) = 0;
        virtual void DrawIndexedBindless(CommandListID commandList, ModelID modelID, u32 numVertices, u32 numInstances) = 0;
        virtual void PopMarker(CommandListID commandList) = 0;
        virtual void PushMarker(CommandListID commandList, Color color, std::string name) = 0;
        virtual void SetConstantBuffer(CommandListID commandListID, u32 slot, void* descriptor, size_t frameIndex) = 0;
        virtual void SetStorageBuffer(CommandListID commandListID, u32 slot, void* descriptor, size_t frameIndex) = 0;
        virtual void BeginPipeline(CommandListID commandList, GraphicsPipelineID pipeline) = 0;
        virtual void EndPipeline(CommandListID commandList, GraphicsPipelineID pipeline) = 0;
        virtual void SetPipeline(CommandListID commandList, ComputePipelineID pipeline) = 0;
        virtual void SetScissorRect(CommandListID commandList, ScissorRect scissorRect) = 0;
        virtual void SetViewport(CommandListID commandList, Viewport viewport) = 0;
        virtual void SetSampler(CommandListID commandList, u32 slot, SamplerID sampler) = 0;
        virtual void SetTexture(CommandListID commandList, u32 slot, TextureID texture) = 0;
        virtual void SetTextureArray(CommandListID commandList, u32 slot, TextureArrayID textureArray) = 0;
        virtual void SetVertexBuffer(CommandListID commandList, u32 slot, ModelID modelID) = 0;
        virtual void SetIndexBuffer(CommandListID commandList, ModelID modelID) = 0;
        virtual void SetBuffer(CommandListID commandList, u32 slot, void* buffer) = 0;

        // Non-commandlist based present functions
        virtual void Present(Window* window, ImageID image) = 0;
        virtual void Present(Window* window, DepthImageID image) = 0;

    protected:
        Renderer() {}; // Pure virtual class, disallow creation of it

        virtual Backend::BufferBackend* CreateBufferBackend(size_t size, Backend::BufferBackend::Type type) = 0;

    protected:
        robin_hood::unordered_map<u32, RenderLayer> _renderLayers;
    };
}