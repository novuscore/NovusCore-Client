#pragma once
#include <NovusTypes.h>

#include "DescriptorSet.h"

// Descriptors
#include "Descriptors/BufferDesc.h"
#include "Descriptors/CommandListDesc.h"
#include "Descriptors/VertexShaderDesc.h"
#include "Descriptors/PixelShaderDesc.h"
#include "Descriptors/ComputeShaderDesc.h"
#include "Descriptors/GraphicsPipelineDesc.h"
#include "Descriptors/ComputePipelineDesc.h"
#include "Descriptors/ImageDesc.h"
#include "Descriptors/DepthImageDesc.h"
#include "Descriptors/TextureDesc.h"
#include "Descriptors/TextureArrayDesc.h"
#include "Descriptors/SamplerDesc.h"
#include "Descriptors/GPUSemaphoreDesc.h"

class Window;

namespace tracy
{
    struct SourceLocationData;
}

namespace Memory
{
    class Allocator;
}

namespace Renderer
{
    class RenderGraph;
    struct RenderGraphDesc;

    class Renderer
    {
    public:
        virtual void InitWindow(Window* window) = 0;
        virtual void Deinit() = 0;

        virtual void ReloadShaders(bool forceRecompileAll) = 0;

        virtual ~Renderer();

        [[nodiscard]] RenderGraph& CreateRenderGraph(RenderGraphDesc& desc);

        // Creation
        virtual [[nodiscard]] BufferID CreateBuffer(BufferDesc& desc) = 0;
        virtual [[nodiscard]] BufferID CreateTemporaryBuffer(BufferDesc& desc, u32 framesLifetime) = 0;
        virtual [[nodiscard]] void QueueDestroyBuffer(BufferID buffer) = 0;

        virtual [[nodiscard]] ImageID CreateImage(ImageDesc& desc) = 0;
        virtual [[nodiscard]] DepthImageID CreateDepthImage(DepthImageDesc& desc) = 0;

        virtual [[nodiscard]] SamplerID CreateSampler(SamplerDesc& sampler) = 0;
        virtual [[nodiscard]] GPUSemaphoreID CreateGPUSemaphore() = 0;

        virtual [[nodiscard]] GraphicsPipelineID CreatePipeline(GraphicsPipelineDesc& desc) = 0;
        virtual [[nodiscard]] ComputePipelineID CreatePipeline(ComputePipelineDesc& desc) = 0;

        virtual [[nodiscard]] TextureArrayID CreateTextureArray(TextureArrayDesc& desc) = 0;

        virtual [[nodiscard]] TextureID CreateDataTexture(DataTextureDesc& desc) = 0;
        virtual [[nodiscard]] TextureID CreateDataTextureIntoArray(DataTextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) = 0;

        // Loading
        virtual [[nodiscard]] TextureID LoadTexture(TextureDesc& desc) = 0;
        virtual [[nodiscard]] TextureID LoadTextureIntoArray(TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) = 0;

        virtual [[nodiscard]] VertexShaderID LoadShader(VertexShaderDesc& desc) = 0;
        virtual [[nodiscard]] PixelShaderID LoadShader(PixelShaderDesc& desc) = 0;
        virtual [[nodiscard]] ComputeShaderID LoadShader(ComputeShaderDesc& desc) = 0;

        // Unloading
        virtual void UnloadTexture(TextureID textureID) = 0;
        virtual void UnloadTexturesInArray(TextureArrayID textureArrayID, u32 unloadStartIndex) = 0;

        // Command List Functions
        virtual [[nodiscard]] CommandListID BeginCommandList() = 0;
        virtual void EndCommandList(CommandListID commandListID) = 0;
        virtual void Clear(CommandListID commandListID, ImageID image, Color color) = 0;
        virtual void Clear(CommandListID commandListID, DepthImageID image, DepthClearFlags clearFlags, f32 depth, u8 stencil) = 0;
        virtual void Draw(CommandListID commandListID, u32 numVertices, u32 numInstances, u32 vertexOffset, u32 instanceOffset) = 0;
        virtual void DrawIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, u32 drawCount) = 0;
        virtual void DrawIndexed(CommandListID commandListID, u32 numIndices, u32 numInstances, u32 indexOffset, u32 vertexOffset, u32 instanceOffset) = 0;
        virtual void DrawIndexedIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, u32 drawCount) = 0;
        virtual void DrawIndexedIndirectCount(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, BufferID drawCountBuffer, u32 drawCountBufferOffset, u32 maxDrawCount) = 0;
        virtual void Dispatch(CommandListID commandListID, u32 threadGroupCountX, u32 threadGroupCountY, u32 threadGroupCountZ) = 0;
        virtual void DispatchIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset) = 0;
        virtual void PopMarker(CommandListID commandListID) = 0;
        virtual void PushMarker(CommandListID commandListID, Color color, std::string name) = 0;
        virtual void BeginPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) = 0;
        virtual void EndPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) = 0;
        virtual void BeginPipeline(CommandListID commandListID, ComputePipelineID pipeline) = 0;
        virtual void EndPipeline(CommandListID commandListID, ComputePipelineID pipeline) = 0;
        virtual void SetScissorRect(CommandListID commandListID, ScissorRect scissorRect) = 0;
        virtual void SetViewport(CommandListID commandListID, Viewport viewport) = 0;
        virtual void SetVertexBuffer(CommandListID commandListID, u32 slot, BufferID bufferID) = 0;
        virtual void SetIndexBuffer(CommandListID commandListID, BufferID bufferID, IndexFormat indexFormat) = 0;
        virtual void SetBuffer(CommandListID commandListID, u32 slot, BufferID buffer) = 0;
        virtual void BindDescriptorSet(CommandListID commandListID, DescriptorSetSlot slot, Descriptor* descriptors, u32 numDescriptors) = 0;
        virtual void MarkFrameStart(CommandListID commandListID, u32 frameIndex) = 0;
        virtual void BeginTrace(CommandListID commandListID, const tracy::SourceLocationData* sourceLocation) = 0;
        virtual void EndTrace(CommandListID commandListID) = 0;
        virtual void AddSignalSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID) = 0;
        virtual void AddWaitSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID) = 0;
        virtual void CopyImage(CommandListID commandListID, ImageID dstImageID, uvec2 dstPos, u32 dstMipLevel, ImageID srcImageID, uvec2 srcPos, u32 srcMipLevel, uvec2 size) = 0;
        virtual void CopyBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range) = 0;
        virtual void PipelineBarrier(CommandListID commandListID, PipelineBarrierType type, BufferID buffer) = 0;
        virtual void ImageBarrier(CommandListID commandListID, ImageID image) = 0;
        virtual void DepthImageBarrier(CommandListID commandListID, DepthImageID image) = 0;
        virtual void PushConstant(CommandListID commandListID, void* data, u32 offset, u32 size) = 0;
        virtual void FillBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, u64 size, u32 data) = 0;
        virtual void UpdateBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, u64 size, void* data) = 0;

        // Present functions
        virtual void Present(Window* window, ImageID image, GPUSemaphoreID semaphoreID = GPUSemaphoreID::Invalid()) = 0;
        virtual void Present(Window* window, DepthImageID image, GPUSemaphoreID semaphoreID = GPUSemaphoreID::Invalid()) = 0;

        // Utils
        virtual [[nodiscard]] ImageDesc GetImageDesc(ImageID ID) = 0;
        virtual [[nodiscard]] DepthImageDesc GetDepthImageDesc(DepthImageID ID) = 0;
        virtual [[nodiscard]] uvec2 GetImageDimension(const ImageID id, u32 mipLevel) = 0;

        virtual void FlipFrame(u32 frameIndex) = 0;

        [[nodiscard]] BufferID CreateBuffer(BufferID bufferID, BufferDesc& desc);
        [[nodiscard]] BufferID CreateAndFillBuffer(BufferID bufferID, BufferDesc desc, void* data, size_t dataSize);
        [[nodiscard]] BufferID CreateAndFillBuffer(BufferDesc desc, void* data, size_t dataSize);
        [[nodiscard]] BufferID CreateAndFillBuffer(BufferID bufferID, BufferDesc desc, const std::function<void(void*)>& callback);
        [[nodiscard]] BufferID CreateAndFillBuffer(BufferDesc desc, const std::function<void(void*)>& callback);
        virtual void CopyBuffer(BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range) = 0;
        
        virtual [[nodiscard]] void* MapBuffer(BufferID buffer) = 0;
        virtual void UnmapBuffer(BufferID buffer) = 0;

        virtual const std::string& GetGPUName() = 0;

        virtual [[nodiscard]] size_t GetVRAMUsage() = 0;
        virtual [[nodiscard]] size_t GetVRAMBudget() = 0;

        virtual void InitImgui() = 0;
        virtual void DrawImgui(CommandListID commandListID) = 0;

        virtual [[nodiscard]] u32 GetNumImages() = 0;
        virtual [[nodiscard]] u32 GetNumDepthImages() = 0;

    protected:
        Renderer() {}; // Pure virtual class, disallow creation of it
    };
}