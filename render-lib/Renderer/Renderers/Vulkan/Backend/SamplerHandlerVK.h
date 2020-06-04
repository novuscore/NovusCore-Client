#pragma once
#include <NovusTypes.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <robin_hood.h>

#include "../../../Descriptors/SamplerDesc.h"
#include "../../../Descriptors/GraphicsPipelineDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        class TextureHandlerVK;
        class PipelineHandlerVK;

        class SamplerHandlerVK
        {
        public:
            SamplerHandlerVK();
            ~SamplerHandlerVK();

            SamplerID CreateSampler(RenderDeviceVK* device, const SamplerDesc& desc);

            //VkDescriptorSet GetCombinedSampler(RenderDeviceVK* device, TextureHandlerVK* textureHandler, PipelineHandlerVK* pipelineHandler, const SamplerID samplerID, const u32 slot, const TextureID textureID, const GraphicsPipelineID pipelineID);

            VkDescriptorSet GetDescriptorSet(const SamplerID samplerID);

            const SamplerDesc& GetSamplerDesc(const SamplerID samplerID);

        private:

            using _SamplerID = type_safe::underlying_type<SamplerID>;
            struct Sampler
            {
                u64 samplerHash;
                SamplerDesc desc;

                VkSampler sampler;

                VkDescriptorSetLayout descriptorSetLayout;
                VkDescriptorPool descriptorPool = NULL;
                VkDescriptorSet descriptorSet;
            };

        private:
            u64 CalculateSamplerHash(const SamplerDesc& desc);
            //bool TryFindExistingSamplerContainer(u64 descHash, size_t& id);

            bool TryFindExistingSampler(u64 descHash, size_t& id);

            VkFilter ToVkFilterMag(SamplerFilter filter);
            VkFilter ToVkFilterMin(SamplerFilter filter);
            bool ToAnisotropyEnabled(SamplerFilter filter);
            VkSamplerAddressMode ToVkSamplerAddressMode(TextureAddressMode mode);
            VkBorderColor ToVkBorderColor(StaticBorderColor borderColor);
            VkCompareOp ToVkCompareOp(ComparisonFunc func);

        private:
            //std::vector<SamplerContainer> _samplerContainers;

            std::vector<Sampler> _samplers;
        };
    }
}