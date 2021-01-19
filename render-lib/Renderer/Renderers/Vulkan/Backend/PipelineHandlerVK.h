#pragma once
#include <NovusTypes.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <robin_hood.h>

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

        struct DescriptorSetLayoutData
        {
            VkDescriptorSetLayoutCreateInfo createInfo;
            std::vector<VkDescriptorSetLayoutBinding> bindings;
        };

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

            const GraphicsPipelineDesc& GetDescriptor(GraphicsPipelineID id) { return _graphicsPipelines[static_cast<gIDType>(id)].desc; }
            const ComputePipelineDesc& GetDescriptor(ComputePipelineID id) { return _computePipelines[static_cast<gIDType>(id)].desc; }

            VkPipeline GetPipeline(GraphicsPipelineID id) { return _graphicsPipelines[static_cast<gIDType>(id)].pipeline; }
            VkPipeline GetPipeline(ComputePipelineID id) { return _computePipelines[static_cast<cIDType>(id)].pipeline; }

            VkRenderPass GetRenderPass(GraphicsPipelineID id) { return _graphicsPipelines[static_cast<gIDType>(id)].renderPass; }
            VkFramebuffer GetFramebuffer(GraphicsPipelineID id) { return _graphicsPipelines[static_cast<gIDType>(id)].framebuffer; }

            DescriptorSetLayoutData& GetDescriptorSetLayoutData(GraphicsPipelineID id, u32 index) { return _graphicsPipelines[static_cast<gIDType>(id)].descriptorSetLayoutDatas[index]; }

            VkDescriptorSetLayout& GetDescriptorSetLayout(GraphicsPipelineID id, u32 index) { return _graphicsPipelines[static_cast<gIDType>(id)].descriptorSetLayouts[index]; }
            VkDescriptorSetLayout& GetDescriptorSetLayout(ComputePipelineID id, u32 index) { return _computePipelines[static_cast<cIDType>(id)].descriptorSetLayouts[index]; }

            VkPipelineLayout& GetPipelineLayout(GraphicsPipelineID id) { return _graphicsPipelines[static_cast<gIDType>(id)].pipelineLayout; }
            VkPipelineLayout& GetPipelineLayout(ComputePipelineID id) { return _computePipelines[static_cast<cIDType>(id)].pipelineLayout; }

            DescriptorSetBuilderVK* GetDescriptorSetBuilder(GraphicsPipelineID id) { return _graphicsPipelines[static_cast<gIDType>(id)].descriptorSetBuilder; }
            DescriptorSetBuilderVK* GetDescriptorSetBuilder(ComputePipelineID id) { return _computePipelines[static_cast<cIDType>(id)].descriptorSetBuilder; }

        private:

            struct GraphicsPipeline
            {
                GraphicsPipelineDesc desc;
                u64 cacheDescHash;

                VkRenderPass renderPass;
                
                VkPipelineLayout pipelineLayout;
                VkPipeline pipeline;

                u32 numRenderTargets = 0;
                VkFramebuffer framebuffer;

                std::vector<DescriptorSetLayoutData> descriptorSetLayoutDatas;
                std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

                std::vector<VkPushConstantRange> pushConstantRanges;

                DescriptorSetBuilderVK* descriptorSetBuilder;
            };

            struct GraphicsPipelineCacheDesc
            {
                GraphicsPipelineDesc::States states;

                ImageID renderTargets[MAX_RENDER_TARGETS] = { ImageID::Invalid(), ImageID::Invalid(), ImageID::Invalid(), ImageID::Invalid(), ImageID::Invalid(), ImageID::Invalid(), ImageID::Invalid(), ImageID::Invalid() };
                DepthImageID depthStencil = DepthImageID::Invalid();
            };

            struct ComputePipelineCacheDesc
            {
                ComputeShaderID shader;
            };

            struct ComputePipeline
            {
                ComputePipelineDesc desc;
                u64 cacheDescHash;

                VkPipelineLayout pipelineLayout;
                VkPipeline pipeline;

                std::vector<DescriptorSetLayoutData> descriptorSetLayoutDatas;
                std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

                std::vector<VkPushConstantRange> pushConstantRanges;

                DescriptorSetBuilderVK* descriptorSetBuilder;
            };

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

            std::vector<GraphicsPipeline> _graphicsPipelines;
            std::vector<ComputePipeline> _computePipelines;
        };
    }
}