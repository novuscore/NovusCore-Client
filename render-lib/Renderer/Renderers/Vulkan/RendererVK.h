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
        class ModelHandlerVK;
        class ShaderHandlerVK;
        class PipelineHandlerVK;
        class CommandListHandlerVK;
        class SamplerHandlerVK;
        class SemaphoreHandlerVK;
        struct BindInfo;
        class DescriptorSetBuilderVK;
        struct SwapChainVK;
    }
    
    class RendererVK : public Renderer
    {
    public:
        RendererVK(TextureDesc& debugTexture);

        void InitWindow(Window* window) override;
        void Deinit() override;

        // Creation
        BufferID CreateBuffer(BufferDesc& desc) override;
        void QueueDestroyBuffer(BufferID buffer) override;

        ImageID CreateImage(ImageDesc& desc) override;
        DepthImageID CreateDepthImage(DepthImageDesc& desc) override;

        SamplerID CreateSampler(SamplerDesc& desc) override;
        GPUSemaphoreID CreateGPUSemaphore() override;

        GraphicsPipelineID CreatePipeline(GraphicsPipelineDesc& desc) override;
        ComputePipelineID CreatePipeline(ComputePipelineDesc& desc) override;

        ModelID CreatePrimitiveModel(PrimitiveModelDesc& desc) override;
        void UpdatePrimitiveModel(ModelID modelID, PrimitiveModelDesc& desc) override;

        TextureArrayID CreateTextureArray(TextureArrayDesc& desc) override;

        TextureID CreateDataTexture(DataTextureDesc& desc) override;
        TextureID CreateDataTextureIntoArray(DataTextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) override;

        DescriptorSetBackend* CreateDescriptorSetBackend() override;

        // Loading
        ModelID LoadModel(ModelDesc& desc) override;

        TextureID LoadTexture(TextureDesc& desc) override;
        TextureID LoadTextureIntoArray(TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex) override;

        VertexShaderID LoadShader(VertexShaderDesc& desc) override;
        PixelShaderID LoadShader(PixelShaderDesc& desc) override;
        ComputeShaderID LoadShader(ComputeShaderDesc& desc) override;

        void FlipFrame(u32 frameIndex) override;

        // Command List Functions
        CommandListID BeginCommandList() override;
        void EndCommandList(CommandListID commandListID) override;
        void Clear(CommandListID commandListID, ImageID image, Color color) override;
        void Clear(CommandListID commandListID, DepthImageID image, DepthClearFlags clearFlags, f32 depth, u8 stencil) override;
        void Draw(CommandListID commandListID, u32 numVertices, u32 numInstances, u32 vertexOffset, u32 instanceOffset) override;
        void DrawBindless(CommandListID commandListID, u32 numVertices, u32 numInstances) override;
        void DrawIndexedBindless(CommandListID commandListID, ModelID modelID, u32 numVertices, u32 numInstances) override;
        void DrawIndexed(CommandListID commandListID, u32 numIndices, u32 numInstances, u32 indexOffset, u32 vertexOffset, u32 instanceOffset) override;
        void DrawIndexedIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, u32 drawCount) override;
        void DrawIndexedIndirectCount(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, BufferID drawCountBuffer, u32 drawCountBufferOffset, u32 maxDrawCount) override;
        void Dispatch(CommandListID commandListID, u32 threadGroupCountX, u32 threadGroupCountY, u32 threadGroupCountZ) override;
        void DispatchIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset) override;
        void PopMarker(CommandListID commandListID) override;
        void PushMarker(CommandListID commandListID, Color color, std::string name) override;
        void BeginPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) override;
        void EndPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) override;
        void SetPipeline(CommandListID commandListID, ComputePipelineID pipeline) override;
        void SetScissorRect(CommandListID commandListID, ScissorRect scissorRect) override;
        void SetViewport(CommandListID commandListID, Viewport viewport) override;
        void SetVertexBuffer(CommandListID commandListID, u32 slot, BufferID bufferID) override;
        void SetIndexBuffer(CommandListID commandListID, BufferID bufferID, IndexFormat indexFormat) override;
        void SetBuffer(CommandListID commandListID, u32 slot, BufferID buffer) override;
        void BindDescriptorSet(CommandListID commandListID, DescriptorSetSlot slot, Descriptor* descriptors, u32 numDescriptors, u32 frameIndex) override;
        void MarkFrameStart(CommandListID commandListID, u32 frameIndex) override;
        void BeginTrace(CommandListID commandListID, const tracy::SourceLocationData* sourceLocation) override;
        void EndTrace(CommandListID commandListID) override;
        void AddSignalSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID) override;
        void AddWaitSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID) override;
        void CopyBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range) override;
        void PipelineBarrier(CommandListID commandListID, PipelineBarrierType type, BufferID buffer) override;

        // Non-commandlist based present functions
        void Present(Window* window, ImageID image, GPUSemaphoreID semaphoreID = GPUSemaphoreID::Invalid()) override;
        void Present(Window* window, DepthImageID image, GPUSemaphoreID semaphoreID = GPUSemaphoreID::Invalid()) override;

        // Utils
        void CopyBuffer(BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range) override;
        void* MapBuffer(BufferID buffer) override;
        void UnmapBuffer(BufferID buffer) override;

    private:
        bool ReflectDescriptorSet(const std::string& name, u32 nameHash, u32 type, i32& set, const std::vector<Backend::BindInfo>& bindInfos, u32& outBindInfoIndex, VkDescriptorSetLayoutBinding* outDescriptorLayoutBinding);
        void BindDescriptor(Backend::DescriptorSetBuilderVK* builder, void* imageInfosArraysVoid, Descriptor& descriptor, u32 frameIndex);

        void RecreateSwapChain(Backend::SwapChainVK* swapChain);

    private:
        Backend::RenderDeviceVK* _device = nullptr;
        Backend::BufferHandlerVK* _bufferHandler = nullptr;
        Backend::ImageHandlerVK* _imageHandler = nullptr;
        Backend::TextureHandlerVK* _textureHandler = nullptr;
        Backend::ModelHandlerVK* _modelHandler = nullptr;
        Backend::ShaderHandlerVK* _shaderHandler = nullptr;
        Backend::PipelineHandlerVK* _pipelineHandler = nullptr;
        Backend::CommandListHandlerVK* _commandListHandler = nullptr;
        Backend::SamplerHandlerVK* _samplerHandler = nullptr;
        Backend::SemaphoreHandlerVK* _semaphoreHandler = nullptr;

        ModelID _boundModelIndexBuffer = ModelID::Invalid(); // TODO: Move these into CommandListHandler I guess?

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
