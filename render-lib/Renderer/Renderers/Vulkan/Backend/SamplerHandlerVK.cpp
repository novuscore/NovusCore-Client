#include "SamplerHandlerVK.h"
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <Utils/XXHash64.h>
#include "RenderDeviceVK.h"
#include "TextureHandlerVK.h"
#include "PipelineHandlerVK.h"

namespace Renderer
{
    namespace Backend
    {
        SamplerHandlerVK::SamplerHandlerVK()
        {

        }

        SamplerHandlerVK::~SamplerHandlerVK()
        {

        }

        SamplerID SamplerHandlerVK::CreateSampler(RenderDeviceVK* device, const SamplerDesc& desc)
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
            samplerInfo.magFilter = ToVkFilterMag(desc.filter);
            samplerInfo.minFilter = ToVkFilterMin(desc.filter);
            samplerInfo.addressModeU = ToVkSamplerAddressMode(desc.addressU);
            samplerInfo.addressModeV = ToVkSamplerAddressMode(desc.addressV);
            samplerInfo.addressModeW = ToVkSamplerAddressMode(desc.addressW);
            samplerInfo.anisotropyEnable = ToAnisotropyEnabled(desc.filter);
            samplerInfo.maxAnisotropy = static_cast<f32>(desc.maxAnisotropy);
            samplerInfo.borderColor = ToVkBorderColor(desc.borderColor);
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = desc.comparisonFunc != ComparisonFunc::COMPARISON_FUNC_ALWAYS;
            samplerInfo.compareOp = ToVkCompareOp(desc.comparisonFunc);
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = desc.mipLODBias;
            samplerInfo.minLod = desc.minLOD;
            samplerInfo.maxLod = desc.maxLOD;

            if (vkCreateSampler(device->_device, &samplerInfo, nullptr, &sampler.sampler) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create sampler!");
            }

            // Create descriptor set layout
            VkDescriptorSetLayoutBinding descriptorLayout = {};
            descriptorLayout.binding = 0;
            descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptorLayout.descriptorCount = 1;
            descriptorLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            descriptorLayout.pImmutableSamplers = NULL;

            VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {};
            descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorLayoutInfo.pNext = NULL;
            descriptorLayoutInfo.bindingCount = 1;
            descriptorLayoutInfo.pBindings = &descriptorLayout;

            if (vkCreateDescriptorSetLayout(device->_device, &descriptorLayoutInfo, NULL, &sampler.descriptorSetLayout) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor set layout for sampler!");
            }

            // Create descriptor pool
            VkDescriptorPoolSize poolSize = {};
            poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
            poolSize.descriptorCount = 1;

            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = 1;

            if (vkCreateDescriptorPool(device->_device, &poolInfo, nullptr, &sampler.descriptorPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor pool for sampler!");
            }

            // Create descriptor set
            VkDescriptorSetAllocateInfo allocInfo;
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.pNext = NULL;
            allocInfo.descriptorPool = sampler.descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &sampler.descriptorSetLayout;

            if (vkAllocateDescriptorSets(device->_device, &allocInfo, &sampler.descriptorSet) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor set for sampler!");
            }

            VkDescriptorImageInfo descriptorInfo = {};
            descriptorInfo.sampler = sampler.sampler;

            VkWriteDescriptorSet descriptorWrite;
            descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = NULL;
            descriptorWrite.dstSet = sampler.descriptorSet;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptorWrite.pImageInfo = &descriptorInfo;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.dstBinding = 0;

            vkUpdateDescriptorSets(device->_device, 1, &descriptorWrite, 0, NULL);

            _samplers.push_back(sampler);
            return SamplerID(static_cast<type>(nextID));
        }

        /*SamplerID SamplerHandlerVK::CreateSampler(RenderDeviceVK* device, const SamplerDesc& desc)
        {
            using type = type_safe::underlying_type<SamplerID>;

            // Check the cache
            size_t nextID;
            u64 samplerHash = CalculateSamplerHash(desc);
            if (TryFindExistingSamplerContainer(samplerHash, nextID))
            {
                return SamplerID(static_cast<type>(nextID));
            }
            nextID = _samplerContainers.size();

            // Make sure we haven't exceeded the limit of the SamplerID type, if this hits you need to change type of SamplerID to something bigger
            assert(nextID < SamplerID::MaxValue());

            SamplerContainer samplerContainer;
            samplerContainer.samplerHash = samplerHash;
            samplerContainer.desc = desc;

            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = ToVkFilterMag(desc.filter);
            samplerInfo.minFilter = ToVkFilterMin(desc.filter);
            samplerInfo.addressModeU = ToVkSamplerAddressMode(desc.addressU);
            samplerInfo.addressModeV = ToVkSamplerAddressMode(desc.addressV);
            samplerInfo.addressModeW = ToVkSamplerAddressMode(desc.addressW);
            samplerInfo.anisotropyEnable = ToAnisotropyEnabled(desc.filter);
            samplerInfo.maxAnisotropy = static_cast<f32>(desc.maxAnisotropy);
            samplerInfo.borderColor = ToVkBorderColor(desc.borderColor);
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = desc.comparisonFunc != ComparisonFunc::COMPARISON_FUNC_ALWAYS;
            samplerInfo.compareOp = ToVkCompareOp(desc.comparisonFunc);
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = desc.mipLODBias;
            samplerInfo.minLod = desc.minLOD;
            samplerInfo.maxLod = desc.maxLOD;

            if (vkCreateSampler(device->_device, &samplerInfo, nullptr, &samplerContainer.sampler) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create texture sampler!");
            }

            _samplerContainers.push_back(samplerContainer);
            return SamplerID(static_cast<type>(nextID));
        }*/

        /*VkDescriptorSet SamplerHandlerVK::GetCombinedSampler(RenderDeviceVK* device, TextureHandlerVK* textureHandler, PipelineHandlerVK* pipelineHandler, const SamplerID samplerID, const u32 slot, const TextureID textureID, const GraphicsPipelineID pipelineID)
        {
            SamplerContainer& samplerContainer = _samplerContainers[static_cast<_SamplerID>(samplerID)];

            VkImageView imageView = textureHandler->GetImageView(textureID);
            
            Backend::DescriptorSetLayoutData& descriptorSetLayoutData = pipelineHandler->GetDescriptorSetLayoutData(pipelineID, slot);
            VkDescriptorSetLayout& descriptorSetLayout = pipelineHandler->GetDescriptorSetLayout(pipelineID, slot);
            
            CombinedSampler& combinedSampler = samplerContainer.combinedSamplers[static_cast<_TextureID>(textureID)];
            
            if (combinedSampler.descriptorPool == NULL)
            {
                VkDescriptorPoolSize poolSize = {};
                poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSize.descriptorCount = 1;

                VkDescriptorPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = 1;
                poolInfo.pPoolSizes = &poolSize;
                poolInfo.maxSets = 1;

                if (vkCreateDescriptorPool(device->_device, &poolInfo, nullptr, &combinedSampler.descriptorPool) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to create descriptor pool for combined sampler!");
                }

                VkDescriptorSetAllocateInfo allocInfo = {};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = combinedSampler.descriptorPool;
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = &descriptorSetLayout;

                if (vkAllocateDescriptorSets(device->_device, &allocInfo, &combinedSampler.descriptorSet) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to allocate descriptor sets!");
                }

                VkDescriptorImageInfo imageInfo = {};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = imageView;
                imageInfo.sampler = samplerContainer.sampler;

                VkWriteDescriptorSet descriptorWrite = {};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = combinedSampler.descriptorSet;
                descriptorWrite.dstBinding = 0;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = nullptr;
                descriptorWrite.pImageInfo = &imageInfo;
                descriptorWrite.pTexelBufferView = nullptr;

                vkUpdateDescriptorSets(device->_device, 1, &descriptorWrite, 0, nullptr);
            }

            return combinedSampler.descriptorSet;
        }*/

        VkDescriptorSet SamplerHandlerVK::GetDescriptorSet(const SamplerID samplerID)
        {
            using type = type_safe::underlying_type<SamplerID>;

            // Lets make sure this id exists
            assert(_samplers.size() > static_cast<type>(samplerID));
            return _samplers[static_cast<type>(samplerID)].descriptorSet;
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

        /*bool SamplerHandlerVK::TryFindExistingSamplerContainer(u64 descHash, size_t& id)
        {
            id = 0;

            for (auto& samplerContainer : _samplerContainers)
            {
                if (descHash == samplerContainer.samplerHash)
                {
                    return true;
                }
                id++;
            }

            return false;
        }*/

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

        VkFilter SamplerHandlerVK::ToVkFilterMag(SamplerFilter filter)
        {
            switch (filter)
            {
                case SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_POINT:                           return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MIN_MAG_POINT_MIP_LINEAR:                    return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:              return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MIN_POINT_MAG_MIP_LINEAR:                    return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MIN_LINEAR_MAG_MIP_POINT:                    return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:             return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MIN_MAG_LINEAR_MIP_POINT:                    return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_LINEAR:                          return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_ANISOTROPIC:                                 return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_POINT:                return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:         return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:   return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:         return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:         return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:  return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:         return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:               return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_ANISOTROPIC:                      return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_MAG_MIP_POINT:                   return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:      return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:            return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:            return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:     return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:                  return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_ANISOTROPIC:                         return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_MAG_MIP_POINT:                   return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:      return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:            return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:            return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:     return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:                  return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_ANISOTROPIC:                         return VK_FILTER_LINEAR;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more filters?");
            }

            return VK_FILTER_NEAREST;
        }

        VkFilter SamplerHandlerVK::ToVkFilterMin(SamplerFilter filter)
        {
            switch (filter)
            {
                case SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_POINT:                           return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MIN_MAG_POINT_MIP_LINEAR:                    return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:              return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MIN_POINT_MAG_MIP_LINEAR:                    return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MIN_LINEAR_MAG_MIP_POINT:                    return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:             return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MIN_MAG_LINEAR_MIP_POINT:                    return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MIN_MAG_MIP_LINEAR:                          return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_ANISOTROPIC:                                 return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_POINT:                return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:         return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:   return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:         return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:         return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:  return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:         return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:               return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_ANISOTROPIC:                      return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_MAG_MIP_POINT:                   return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:      return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:     return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:                  return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_ANISOTROPIC:                         return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_MAG_MIP_POINT:                   return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:      return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:     return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:                  return VK_FILTER_LINEAR;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_ANISOTROPIC:                         return VK_FILTER_LINEAR;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more filters?");
            }

            return VK_FILTER_NEAREST;
        }

        bool SamplerHandlerVK::ToAnisotropyEnabled(SamplerFilter filter)
        {
            switch (filter)
            {
                case SamplerFilter::SAMPLER_FILTER_ANISOTROPIC:             return true;
                case SamplerFilter::SAMPLER_FILTER_COMPARISON_ANISOTROPIC:  return true;
                case SamplerFilter::SAMPLER_FILTER_MINIMUM_ANISOTROPIC:     return true;
                case SamplerFilter::SAMPLER_FILTER_MAXIMUM_ANISOTROPIC:     return true;
            }
            return false;
        }

        VkSamplerAddressMode SamplerHandlerVK::ToVkSamplerAddressMode(TextureAddressMode mode)
        {
            switch (mode)
            {
                case TextureAddressMode::TEXTURE_ADDRESS_MODE_WRAP:         return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case TextureAddressMode::TEXTURE_ADDRESS_MODE_MIRROR:       return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                case TextureAddressMode::TEXTURE_ADDRESS_MODE_CLAMP:        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                case TextureAddressMode::TEXTURE_ADDRESS_MODE_BORDER:       return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                case TextureAddressMode::TEXTURE_ADDRESS_MODE_MIRROR_ONCE:  return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more modes?");
            }

            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }

        VkBorderColor SamplerHandlerVK::ToVkBorderColor(StaticBorderColor borderColor)
        {
            switch (borderColor)
            {
                case StaticBorderColor::STATIC_BORDER_COLOR_TRANSPARENT_BLACK:  return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
                case StaticBorderColor::STATIC_BORDER_COLOR_OPAQUE_BLACK:       return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
                case StaticBorderColor::STATIC_BORDER_COLOR_OPAQUE_WHITE:       return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more colors?");
            }

            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        }

        VkCompareOp SamplerHandlerVK::ToVkCompareOp(ComparisonFunc func)
        {
            switch (func)
            {
                case ComparisonFunc::COMPARISON_FUNC_NEVER:             return VK_COMPARE_OP_NEVER;
                case ComparisonFunc::COMPARISON_FUNC_LESS:              return VK_COMPARE_OP_LESS;
                case ComparisonFunc::COMPARISON_FUNC_EQUAL:             return VK_COMPARE_OP_EQUAL;
                case ComparisonFunc::COMPARISON_FUNC_LESS_EQUAL:        return VK_COMPARE_OP_LESS_OR_EQUAL;
                case ComparisonFunc::COMPARISON_FUNC_GREATER:           return VK_COMPARE_OP_GREATER;
                case ComparisonFunc::COMPARISON_FUNC_NOT_EQUAL:         return VK_COMPARE_OP_NOT_EQUAL;
                case ComparisonFunc::COMPARISON_FUNC_GREATER_EQUAL:     return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case ComparisonFunc::COMPARISON_FUNC_ALWAYS:            return VK_COMPARE_OP_ALWAYS;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more comparison funcs?");
            }

            return VK_COMPARE_OP_NEVER;
        }



    }
}