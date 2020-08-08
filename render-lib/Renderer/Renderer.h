#pragma once
#include <NovusTypes.h>
#include <Utils/StringUtils.h>
#include <robin_hood.h>
#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include "RenderPass.h"
#include "RenderStates.h"
#include "Font.h"
#include "DescriptorSet.h"

// Descriptors
#include "Descriptors/BufferDesc.h"
#include "Descriptors/CommandListDesc.h"
#include "Descriptors/VertexShaderDesc.h"
#include "Descriptors/PixelShaderDesc.h"
#include "Descriptors/ComputeShaderDesc.h"
#include "Descriptors/ImageDesc.h"
#include "Descriptors/TextureDesc.h"
#include "Descriptors/TextureArrayDesc.h"
#include "Descriptors/DepthImageDesc.h"
#include "Descriptors/ModelDesc.h"
#include "Descriptors/SamplerDesc.h"
#include "Descriptors/GPUSemaphoreDesc.h"
#include "Descriptors/FontDesc.h"

class Window;

namespace tracy
{
    struct SourceLocationData;
}

namespace Renderer
{
    class Renderer
    {
    public:
        virtual void InitWindow(Window* window) = 0;
        virtual void Deinit() = 0;

        virtual ~Renderer();

        RenderGraph CreateRenderGraph(RenderGraphDesc& desc);

        // Creation
        virtual BufferID CreateBuffer(BufferDesc& desc) = 0;
        virtual void QueueDestroyBuffer(BufferID buffer) = 0;

        virtual ImageID CreateImage(ImageDesc& desc) = 0;
        virtual DepthImageID CreateDepthImage(DepthImageDesc& desc) = 0;

        virtual SamplerID CreateSampler(SamplerDesc& sampler) = 0;
        virtual GPUSemaphoreID CreateGPUSemaphore() = 0;

        virtual GraphicsPipelineID CreatePipeline(GraphicsPipelineDesc& desc) = 0;
        virtual ComputePipelineID CreatePipeline(ComputePipelineDesc& desc) = 0;

        virtual ModelID CreatePrimitiveModel(PrimitiveModelDesc& desc) = 0;
        virtual void UpdatePrimitiveModel(ModelID model, PrimitiveModelDesc& desc) = 0;

        virtual TextureArrayID CreateTextureArray(TextureArrayDesc& desc) = 0;

        virtual TextureID CreateDataTexture(DataTextureDesc& desc) = 0;
        virtual TextureID CreateDataTextureIntoArray(DataTextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) = 0;
        
        virtual DescriptorSetBackend* CreateDescriptorSetBackend() = 0;

        // Loading
        virtual ModelID LoadModel(ModelDesc& desc) = 0;

        virtual TextureID LoadTexture(TextureDesc& desc) = 0;
        virtual TextureID LoadTextureIntoArray(TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) = 0;

        virtual VertexShaderID LoadShader(VertexShaderDesc& desc) = 0;
        virtual PixelShaderID LoadShader(PixelShaderDesc& desc) = 0;
        virtual ComputeShaderID LoadShader(ComputeShaderDesc& desc) = 0;

        virtual void FlipFrame(u32 frameIndex) = 0;

        // Command List Functions
        virtual CommandListID BeginCommandList() = 0;
        virtual void EndCommandList(CommandListID commandListID) = 0;
        virtual void Clear(CommandListID commandListID, ImageID image, Color color) = 0;
        virtual void Clear(CommandListID commandListID, DepthImageID image, DepthClearFlags clearFlags, f32 depth, u8 stencil) = 0;
        virtual void Draw(CommandListID commandListID, u32 numVertices, u32 numInstances, u32 vertexOffset, u32 instanceOffset) = 0;
        virtual void DrawBindless(CommandListID commandListID, u32 numVertices, u32 numInstances) = 0;
        virtual void DrawIndexedBindless(CommandListID commandListID, ModelID modelID, u32 numVertices, u32 numInstances) = 0;
        virtual void DrawIndexed(CommandListID commandListID, u32 numIndices, u32 numInstances, u32 indexOffset, u32 vertexOffset, u32 instanceOffset) = 0;
        virtual void DrawIndexedIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, u32 drawCount) = 0;
        virtual void DrawIndexedIndirectCount(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, BufferID drawCountBuffer, u32 drawCountBufferOffset, u32 maxDrawCount) = 0;
        virtual void PopMarker(CommandListID commandListID) = 0;
        virtual void PushMarker(CommandListID commandListID, Color color, std::string name) = 0;
        virtual void BeginPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) = 0;
        virtual void EndPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) = 0;
        virtual void SetPipeline(CommandListID commandListID, ComputePipelineID pipeline) = 0;
        virtual void SetScissorRect(CommandListID commandListID, ScissorRect scissorRect) = 0;
        virtual void SetViewport(CommandListID commandListID, Viewport viewport) = 0;
        virtual void SetVertexBuffer(CommandListID commandListID, u32 slot, BufferID bufferID) = 0;
        virtual void SetIndexBuffer(CommandListID commandListID, BufferID bufferID, IndexFormat indexFormat) = 0;
        virtual void SetBuffer(CommandListID commandListID, u32 slot, BufferID buffer) = 0;
        virtual void BindDescriptorSet(CommandListID commandListID, DescriptorSetSlot slot, Descriptor* descriptors, u32 numDescriptors, u32 frameIndex) = 0;
        virtual void MarkFrameStart(CommandListID commandListID, u32 frameIndex) = 0;
        virtual void BeginTrace(CommandListID commandListID, const tracy::SourceLocationData* sourceLocation) = 0;
        virtual void EndTrace(CommandListID commandListID) = 0;
        virtual void AddSignalSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID) = 0;
        virtual void AddWaitSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID) = 0;
        virtual void CopyBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range) = 0;

        // Present functions
        virtual void Present(Window* window, ImageID image, GPUSemaphoreID semaphoreID = GPUSemaphoreID::Invalid()) = 0;
        virtual void Present(Window* window, DepthImageID image, GPUSemaphoreID semaphoreID = GPUSemaphoreID::Invalid()) = 0;

        // Utils
        virtual void CopyBuffer(BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range) = 0;
        virtual void* MapBuffer(BufferID buffer) = 0;
        virtual void UnmapBuffer(BufferID buffer) = 0;

    protected:
        Renderer() {}; // Pure virtual class, disallow creation of it
    };
}