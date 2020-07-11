#pragma once
#include "../../Renderer.h"

struct VkDescriptorSetLayoutBinding;

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        class ImageHandlerVK;
        class TextureHandlerVK;
        class ModelHandlerVK;
        class ShaderHandlerVK;
        class PipelineHandlerVK;
        class CommandListHandlerVK;
        class SamplerHandlerVK;
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
        ImageID CreateImage(ImageDesc& desc) override;
        DepthImageID CreateDepthImage(DepthImageDesc& desc) override;

        SamplerID CreateSampler(SamplerDesc& desc) override;

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

        // Command List Functions
        CommandListID BeginCommandList() override;
        void EndCommandList(CommandListID commandListID) override;
        void Clear(CommandListID commandListID, ImageID image, Color color) override;
        void Clear(CommandListID commandListID, DepthImageID image, DepthClearFlags clearFlags, f32 depth, u8 stencil) override;
        void Draw(CommandListID commandListID, ModelID modelID) override;
        void DrawBindless(CommandListID commandListID, u32 numVertices, u32 numInstances) override;
        void DrawIndexedBindless(CommandListID commandListID, ModelID modelID, u32 numVertices, u32 numInstances) override;
        void PopMarker(CommandListID commandListID) override;
        void PushMarker(CommandListID commandListID, Color color, std::string name) override;
        void BeginPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) override;
        void EndPipeline(CommandListID commandListID, GraphicsPipelineID pipeline) override;
        void SetPipeline(CommandListID commandListID, ComputePipelineID pipeline) override;
        void SetScissorRect(CommandListID commandListID, ScissorRect scissorRect) override;
        void SetViewport(CommandListID commandListID, Viewport viewport) override;
        void SetVertexBuffer(CommandListID commandListID, u32 slot, ModelID modelID) override;
        void SetIndexBuffer(CommandListID commandListID, ModelID modelID) override;
        void SetBuffer(CommandListID commandListID, u32 slot, void* buffer) override;
        void BindDescriptorSet(CommandListID commandListID, DescriptorSetSlot slot, Descriptor* descriptors, u32 numDescriptors, u32 frameIndex) override;
        void MarkFrameStart(CommandListID commandListID, u32 frameIndex) override;

        // Non-commandlist based present functions
        void Present(Window* window, ImageID image) override;
        void Present(Window* window, DepthImageID image) override;
        
    protected:
        Backend::BufferBackend* CreateBufferBackend(size_t size, Backend::BufferBackend::Type type) override;

    private:
        bool ReflectDescriptorSet(const std::string& name, u32 nameHash, u32 type, i32& set, const std::vector<Backend::BindInfo>& bindInfos, u32& outBindInfoIndex, VkDescriptorSetLayoutBinding* outDescriptorLayoutBinding);
        void BindDescriptor(Backend::DescriptorSetBuilderVK* builder, void* imageInfosArraysVoid, Descriptor& descriptor, u32 frameIndex);

        void RecreateSwapChain(Backend::SwapChainVK* swapChain);

    private:
        Backend::RenderDeviceVK* _device = nullptr;
        Backend::ImageHandlerVK* _imageHandler = nullptr;
        Backend::TextureHandlerVK* _textureHandler = nullptr;
        Backend::ModelHandlerVK* _modelHandler = nullptr;
        Backend::ShaderHandlerVK* _shaderHandler = nullptr;
        Backend::PipelineHandlerVK* _pipelineHandler = nullptr;
        Backend::CommandListHandlerVK* _commandListHandler = nullptr;
        Backend::SamplerHandlerVK* _samplerHandler = nullptr;

        ModelID _boundModelIndexBuffer = ModelID::Invalid(); // TODO: Move these into CommandListHandler I guess?

        i8 _renderPassOpenCount = 0; // TODO: Move these into CommandListHandler I guess?
    };
}
