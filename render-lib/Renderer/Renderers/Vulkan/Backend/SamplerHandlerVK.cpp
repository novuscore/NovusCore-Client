#include "SamplerHandlerVK.h"
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <Utils/XXHash64.h>
#include "RenderDeviceVK.h"
#include "TextureHandlerVK.h"
#include "PipelineHandlerVK.h"
#include "FormatConverterVK.h"

namespace Renderer
{
    namespace Backend
    {
        void SamplerHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;
        }

        SamplerID SamplerHandlerVK::CreateSampler(const SamplerDesc& desc)
        {
            using type = type_safe::underlying_type<SamplerID>;

            // Check the cache
            size_t nextID;
            u64 samplerHash = CalculateSamplerHash(desc);
            if (TryFindExistingSampler(samplerHash, nextID))
            {
                return SamplerID(static_cast<type>(nextID));
            }
            nextID = _samplers.size();

            // Make sure we haven't exceeded the limit of the SamplerID type, if this hits you need to change type of SamplerID to something bigger
            assert(nextID < SamplerID::MaxValue());

            Sampler sampler;
            sampler.samplerHash = samplerHash;
            sampler.desc = desc;

            // Create sampler
            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = FormatConverterVK::ToVkFilterMag(desc.filter);
            samplerInfo.minFilter = FormatConverterVK::ToVkFilterMin(desc.filter);
            samplerInfo.addressModeU = FormatConverterVK::ToVkSamplerAddressMode(desc.addressU);
            samplerInfo.addressModeV = FormatConverterVK::ToVkSamplerAddressMode(desc.addressV);
            samplerInfo.addressModeW = FormatConverterVK::ToVkSamplerAddressMode(desc.addressW);
            samplerInfo.anisotropyEnable = FormatConverterVK::ToAnisotropyEnabled(desc.filter);
            samplerInfo.maxAnisotropy = static_cast<f32>(desc.maxAnisotropy);
            samplerInfo.borderColor = FormatConverterVK::ToVkBorderColor(desc.borderColor);
            samplerInfo.unnormalizedCoordinates = desc.unnormalizedCoordinates;
            samplerInfo.compareEnable = desc.comparisonFunc != ComparisonFunc::COMPARISON_FUNC_ALWAYS;
            samplerInfo.compareOp = FormatConverterVK::ToVkCompareOp(desc.comparisonFunc);
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = desc.mipLODBias;
            samplerInfo.minLod = desc.minLOD;
            samplerInfo.maxLod = desc.maxLOD;

            if (vkCreateSampler(_device->_device, &samplerInfo, nullptr, &sampler.sampler) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create sampler!");
            }

            _samplers.push_back(sampler);
            return SamplerID(static_cast<type>(nextID));
        }

        VkSampler& SamplerHandlerVK::GetSampler(const SamplerID samplerID)
        {
            using type = type_safe::underlying_type<SamplerID>;

            // Lets make sure this id exists
            assert(_samplers.size() > static_cast<type>(samplerID));
            return _samplers[static_cast<type>(samplerID)].sampler;
        }

        const SamplerDesc& SamplerHandlerVK::GetSamplerDesc(const SamplerID samplerID)
        {
            using type = type_safe::underlying_type<SamplerID>;

            // Lets make sure this id exists
            assert(_samplers.size() > static_cast<type>(samplerID));
            return _samplers[static_cast<type>(samplerID)].desc;
        }

        u64 SamplerHandlerVK::CalculateSamplerHash(const SamplerDesc& desc)
        {
            return XXHash64::hash(&desc, sizeof(desc), 0);
        }

        bool SamplerHandlerVK::TryFindExistingSampler(u64 descHash, size_t& id)
        {
            id = 0;

            for (auto& sampler : _samplers)
            {
                if (descHash == sampler.samplerHash)
                {
                    return true;
                }
                id++;
            }

            return false;
        }
    }
}