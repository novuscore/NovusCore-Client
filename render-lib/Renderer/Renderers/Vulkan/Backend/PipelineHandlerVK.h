#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan_core.h>
#include <vector>

#include "../../../Descriptors/GraphicsPipelineDesc.h"
#include "../../../Descriptors/ComputePipelineDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        class ShaderHandlerVK;
        class ImageHandlerVK;
        class DescriptorSetBuilderVK;
        struct GraphicsPipeline;

        struct DescriptorSetLayoutData
        {
            VkDescriptorSetLayoutCreateInfo createInfo;
            std::vector<VkDescriptorSetLayoutBinding> bindings;
        };

        struct IPipelineHandlerVKData {};

        class PipelineHandlerVK
        {
            using gIDType = type_safe::underlying_type<GraphicsPipelineID>;
            using cIDType = type_safe::underlying_type<ComputePipelineID>;
        public:
            void Init(RenderDeviceVK* device, ShaderHandlerVK* shaderHandler, ImageHandlerVK* imageHandler);
            void DiscardPipelines();

            void OnWindowResize();

            GraphicsPipelineID CreatePipeline(const GraphicsPipelineDesc& desc);
            ComputePipelineID CreatePipeline(const ComputePipelineDesc& desc);

            const GraphicsPipelineDesc& GetDescriptor(GraphicsPipelineID id);
            const ComputePipelineDesc& GetDescriptor(ComputePipelineID id);

            // Be careful with these
            GraphicsPipelineDesc& GetMutableDescriptor(GraphicsPipelineID id);
            ComputePipelineDesc& GetMutableDescriptor(ComputePipelineID id);

            VkPipeline GetPipeline(GraphicsPipelineID id);
            VkPipeline GetPipeline(ComputePipelineID id);

            VkRenderPass GetRenderPass(GraphicsPipelineID id);
            VkFramebuffer GetFramebuffer(GraphicsPipelineID id);

            DescriptorSetLayoutData& GetDescriptorSetLayoutData(GraphicsPipelineID id, u32 index);

            VkDescriptorSetLayout& GetDescriptorSetLayout(GraphicsPipelineID id, u32 index);
            VkDescriptorSetLayout& GetDescriptorSetLayout(ComputePipelineID id, u32 index);

            VkPipelineLayout& GetPipelineLayout(GraphicsPipelineID id);
            VkPipelineLayout& GetPipelineLayout(ComputePipelineID id);

            DescriptorSetBuilderVK* GetDescriptorSetBuilder(GraphicsPipelineID id);
            DescriptorSetBuilderVK* GetDescriptorSetBuilder(ComputePipelineID id);

        private:
            u64 CalculateCacheDescHash(const GraphicsPipelineDesc& desc);
            u64 CalculateCacheDescHash(const ComputePipelineDesc& desc);
            bool TryFindExistingGPipeline(u64 descHash, size_t& id);
            bool TryFindExistingCPipeline(u64 descHash, size_t& id);
            DescriptorSetLayoutData& GetDescriptorSet(i32 setNumber, std::vector<DescriptorSetLayoutData>& sets);
            
            void CreateFramebuffer(GraphicsPipeline& pipeline);

        private:
            RenderDeviceVK* _device;
            ImageHandlerVK* _imageHandler;
            ShaderHandlerVK* _shaderHandler;

            IPipelineHandlerVKData* _data;
        };
    }
}