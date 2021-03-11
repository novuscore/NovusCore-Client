#pragma once
#include "../../Renderer.h"
#include <array>

struct VkDescriptorSetLayoutBinding;

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        class BufferHandlerVK;
        class ImageHandlerVK;
        class TextureHandlerVK;
        class ShaderHandlerVK;
        class PipelineHandlerVK;
        class CommandListHandlerVK;
        class SamplerHandlerVK;
        class SemaphoreHandlerVK;
        struct BindInfo;
        class DescriptorSetBuilderVK;
        struct SwapChainVK;
        class DescriptorSetBuilderVK;
    }
    
    class RendererVK : public Renderer
    {
    public:
        RendererVK(TextureDesc& debugTexture);

        void InitWindow(Window* window) override;
        void Deinit() override;

        void ReloadShaders(bool forceRecompileAll) override;

        // Creation
        BufferID CreateBuffer(BufferDesc& desc) override;
        BufferID CreateTemporaryBuffer(BufferDesc& desc, u32 framesLifetime) override;
        void QueueDestroyBuffer(BufferID buffer) override;

        ImageID CreateImage(ImageDesc& desc) override;
        DepthImageID CreateDepthImage(DepthImageDesc& desc) override;

        SamplerID CreateSampler(SamplerDesc& desc) override;
        GPUSemaphoreID CreateGPUSemaphore() override;

        GraphicsPipelineID CreatePipeline(GraphicsPipelineDesc& desc) override;
        ComputePipelineID CreatePipeline(ComputePipelineDesc& desc) override;

        TextureArrayID CreateTextureArray(TextureArrayDesc& desc) override;

        TextureID CreateDataTexture(DataTextureDesc& desc) override;
        TextureID CreateDataTextureIntoArray(DataTextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) override;

        // Loading
        TextureID LoadTexture(TextureDesc& desc) override;
        TextureID LoadTextureIntoArray(TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) override;

        VertexShaderID LoadShader(VertexShaderDesc& desc) override;
        PixelShaderID LoadShader(PixelShaderDesc& desc) override;
        ComputeShaderID LoadShader(ComputeShaderDesc& desc) override;

        // Unloading
        void UnloadTexture(TextureID textureID) override;
        void UnloadTexturesInArray(TextureArrayID textureArrayID, u32 unloadStartIndex) override;

        // Command List Functions
        CommandListID BeginCommandList() override;
        void EndCommandList(CommandListID commandListID) override;
        void Clear(CommandListID commandListID, ImageID image, Color color) override;
        void Clear(CommandListID commandListID, DepthImageID image, DepthClearFlags clearFlags, f32 depth, u8 stencil) override;
        void Draw(CommandListID commandListID, u32 numVertices, u32 numInstances, u32 vertexOffset, u32 instanceOffset) override;
        void DrawIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, u32 drawCount) override;
        void DrawIndexed(CommandListID commandListID, u32 numIndices, u32 numInstances, u32 indexOffset, u32 vertexOffset, u32 instanceOffset) override;
        void DrawIndexedIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, u32 drawCount) override;
        void DrawIndexedIndirectCount(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, BufferID drawCountBuffer, u32 drawCountBufferOffset, u32 maxDrawCount) override;
        void Dispatch(CommandListID commandListID, u32 threadGroupCountX, u32 threadGroupCountY, u32 threadGroupCountZ) override;
        void DispatchIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset) override;
        void PopMarker(CommandListID commandListID) override;
        void PushMarker(CommandListID commandListID, Color color, std::string name) override;
        void BeginPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) override;
        void EndPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) override;
        void BeginPipeline(CommandListID commandListID, ComputePipelineID pipeline) override;
        void EndPipeline(CommandListID commandListID, ComputePipelineID pipeline) override;
        void SetScissorRect(CommandListID commandListID, ScissorRect scissorRect) override;
        void SetViewport(CommandListID commandListID, Viewport viewport) override;
        void SetVertexBuffer(CommandListID commandListID, u32 slot, BufferID bufferID) override;
        void SetIndexBuffer(CommandListID commandListID, BufferID bufferID, IndexFormat indexFormat) override;
        void SetBuffer(CommandListID commandListID, u32 slot, BufferID buffer) override;
        void BindDescriptorSet(CommandListID commandListID, DescriptorSetSlot slot, Descriptor* descriptors, u32 numDescriptors) override;
        void MarkFrameStart(CommandListID commandListID, u32 frameIndex) override;
        void BeginTrace(CommandListID commandListID, const tracy::SourceLocationData* sourceLocation) override;
        void EndTrace(CommandListID commandListID) override;
        void AddSignalSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID) override;
        void AddWaitSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID) override;
        void CopyImage(CommandListID commandListID, ImageID dstImageID, uvec2 dstPos, u32 dstMipLevel, ImageID srcImageID, uvec2 srcPos, u32 srcMipLevel, uvec2 size) override;
        void CopyBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range) override;
        void PipelineBarrier(CommandListID commandListID, PipelineBarrierType type, BufferID buffer) override;
        void ImageBarrier(CommandListID commandListID, ImageID image) override;
        void DepthImageBarrier(CommandListID commandListID, DepthImageID image) override;
        void PushConstant(CommandListID commandListID, void* data, u32 offset, u32 size) override;
        void FillBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, u64 size, u32 data) override;
        void UpdateBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, u64 size, void* data) override;

        // Non-commandlist based present functions
        void Present(Window* window, ImageID image, GPUSemaphoreID semaphoreID = GPUSemaphoreID::Invalid()) override;
        void Present(Window* window, DepthImageID image, GPUSemaphoreID semaphoreID = GPUSemaphoreID::Invalid()) override;

        // Utils
        void FlipFrame(u32 frameIndex) override;

        ImageDesc GetImageDesc(ImageID ID) override;
        DepthImageDesc GetDepthImageDesc(DepthImageID ID) override;

        uvec2 GetImageDimension(const ImageID id, u32 mipLevel) override;

        void CopyBuffer(BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range) override;

        void* MapBuffer(BufferID buffer) override;
        void UnmapBuffer(BufferID buffer) override;

        const std::string& GetGPUName() override;

        size_t GetVRAMUsage() override;
        size_t GetVRAMBudget() override;

        void InitImgui() override;
        void DrawImgui(CommandListID commandListID) override;

        u32 GetNumImages() override;
        u32 GetNumDepthImages() override;

    private:
        bool ReflectDescriptorSet(const std::string& name, u32 nameHash, u32 type, i32& set, const std::vector<Backend::BindInfo>& bindInfos, u32& outBindInfoIndex, VkDescriptorSetLayoutBinding* outDescriptorLayoutBinding);
        void BindDescriptor(Backend::DescriptorSetBuilderVK* builder, void* imageInfosArraysVoid, Descriptor& descriptor);

        void RecreateSwapChain(Backend::SwapChainVK* swapChain);
        void CreateDummyPipeline();

    private:
        Backend::RenderDeviceVK* _device = nullptr;
        Backend::BufferHandlerVK* _bufferHandler = nullptr;
        Backend::ImageHandlerVK* _imageHandler = nullptr;
        Backend::TextureHandlerVK* _textureHandler = nullptr;
        Backend::ShaderHandlerVK* _shaderHandler = nullptr;
        Backend::PipelineHandlerVK* _pipelineHandler = nullptr;
        Backend::CommandListHandlerVK* _commandListHandler = nullptr;
        Backend::SamplerHandlerVK* _samplerHandler = nullptr;
        Backend::SemaphoreHandlerVK* _semaphoreHandler = nullptr;

        GraphicsPipelineID _globalDummyPipeline = GraphicsPipelineID::Invalid();
        Backend::DescriptorSetBuilderVK* _descriptorSetBuilder = nullptr;

        Viewport _lastViewport;
        ScissorRect _lastScissorRect;

        i8 _renderPassOpenCount = 0; // TODO: Move these into CommandListHandler I guess?

        struct ObjectDestroyList
        {
            std::vector<BufferID> buffers;
        };

        std::array<ObjectDestroyList, 4> _destroyLists;
        size_t _destroyListIndex = 0;

        void DestroyObjects(ObjectDestroyList& destroyList);
    };
}
