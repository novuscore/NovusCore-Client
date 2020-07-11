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
            void Init(RenderDeviceVK* device);

            SamplerID CreateSampler(const SamplerDesc& desc);

            VkSampler& GetSampler(const SamplerID samplerID);

            const SamplerDesc& GetSamplerDesc(const SamplerID samplerID);
            
        private:
            using _SamplerID = type_safe::underlying_type<SamplerID>;
            struct Sampler
            {
                u64 samplerHash;
                SamplerDesc desc;

                VkSampler sampler;
            };

        private:
            u64 CalculateSamplerHash(const SamplerDesc& desc);

            bool TryFindExistingSampler(u64 descHash, size_t& id);

        private:
            RenderDeviceVK* _device;

            std::vector<Sampler> _samplers;
        };
    }
}