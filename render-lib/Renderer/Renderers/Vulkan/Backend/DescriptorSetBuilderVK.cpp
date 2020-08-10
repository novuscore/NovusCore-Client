#include "DescriptorSetBuilderVK.h"
#include "PipelineHandlerVK.h"
#include "ShaderHandlerVK.h"
#include "RenderDeviceVK.h"
#include <Utils/StringUtils.h>

namespace Renderer
{
    namespace Backend
    {
        DescriptorSetBuilderVK::DescriptorSetBuilderVK(GraphicsPipelineID pipelineID, PipelineHandlerVK* pipelineHandler, ShaderHandlerVK* shaderHandler, DescriptorMegaPoolVK* parentPool)
        {
            _pipelineType = PipelineType::Graphics;
            _pipelineHandler = pipelineHandler;
            _shaderHandler = shaderHandler;
            _parentPool = parentPool;
            _graphicsPipelineID = pipelineID;
        }

        DescriptorSetBuilderVK::DescriptorSetBuilderVK(ComputePipelineID pipelineID, PipelineHandlerVK* pipelineHandler, ShaderHandlerVK* shaderHandler, DescriptorMegaPoolVK* parentPool)
        {
            _pipelineType = PipelineType::Compute;
            _pipelineHandler = pipelineHandler;
            _shaderHandler = shaderHandler;
            _parentPool = parentPool;
            _computePipelineID = pipelineID;
        }

        void DescriptorSetBuilderVK::InitReflectData()
        {
            if (_pipelineType == PipelineType::Graphics)
            {
                // Graphics pipeline
                GraphicsPipelineDesc desc = _pipelineHandler->GetDescriptor(_graphicsPipelineID);

                if (desc.states.vertexShader != VertexShaderID::Invalid())
                {
                    const Backend::BindReflection& bindReflection = _shaderHandler->GetBindReflection(desc.states.vertexShader);
                    _bindInfos.insert(_bindInfos.end(), bindReflection.dataBindings.begin(), bindReflection.dataBindings.end());
                }
                if (desc.states.pixelShader != PixelShaderID::Invalid())
                {
                    const Backend::BindReflection& bindReflection = _shaderHandler->GetBindReflection(desc.states.pixelShader);
                    _bindInfos.insert(_bindInfos.end(), bindReflection.dataBindings.begin(), bindReflection.dataBindings.end());
                }
            }
            else
            {
                // Compute pipeline
                ComputePipelineDesc desc = _pipelineHandler->GetDescriptor(_computePipelineID);

                const Backend::BindReflection& bindReflection = _shaderHandler->GetBindReflection(desc.computeShader);
                _bindInfos.insert(_bindInfos.end(), bindReflection.dataBindings.begin(), bindReflection.dataBindings.end());
            }
        }

        void DescriptorSetBuilderVK::BindSampler(i32 set, i32 binding, VkDescriptorImageInfo& imageInfo)
        {
            for (auto& imageWrite : _imageWrites)
            {
                if (imageWrite.dstBinding == binding && imageWrite.dstSet == set)
                {
                    imageWrite.imageInfo = imageInfo;
                    return;
                }
            }

            ImageWriteDescriptor newWrite;
            newWrite.dstSet = set;
            newWrite.dstBinding = binding;
            newWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            newWrite.imageInfo = imageInfo;
            
            _imageWrites.push_back(newWrite);
        }

        void DescriptorSetBuilderVK::BindSampler(u32 nameHash, VkDescriptorImageInfo& imageInfo)
        {
            for (auto& bindInfo : _bindInfos)
            {
                if (nameHash == bindInfo.nameHash)
                {
                    BindSampler(bindInfo.set, bindInfo.binding, imageInfo);
                    return;
                }
            }
        }

        void DescriptorSetBuilderVK::BindImageArray(i32 set, i32 binding, VkDescriptorImageInfo* images, i32 count)
        {
            for (auto& imageWrite : _imageWrites)
            {
                if (imageWrite.dstBinding == binding && imageWrite.dstSet == set)
                {
                    imageWrite.imageArray = images;
                    imageWrite.imageCount = count;
                    return;
                }
            }

            ImageWriteDescriptor newWrite;
            newWrite.dstSet = set;
            newWrite.dstBinding = binding;
            newWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            newWrite.imageArray = images;
            newWrite.imageCount = count;

            _imageWrites.push_back(newWrite);
        }

        void DescriptorSetBuilderVK::BindImageArray(u32 nameHash, VkDescriptorImageInfo* images, i32 count)
        {
            for (auto& bindInfo : _bindInfos)
            {
                if (nameHash == bindInfo.nameHash)
                {
                    BindImageArray(bindInfo.set, bindInfo.binding, images, count);
                    return;
                }
            }
        }

        void DescriptorSetBuilderVK::BindImage(i32 set, i32 binding, const VkDescriptorImageInfo& imageInfo, bool imageWrite)
        {
            for (auto& imageWrite : _imageWrites) 
            {
                if (imageWrite.dstBinding == binding && imageWrite.dstSet == set)
                {
                    imageWrite.imageInfo = imageInfo;
                    return;
                }
            }

            ImageWriteDescriptor newWrite;
            newWrite.dstSet = set;
            newWrite.dstBinding = binding;
            newWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            newWrite.imageInfo = imageInfo;
            if (imageWrite)
            {
                newWrite.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }

            _imageWrites.push_back(newWrite);
        }

        void DescriptorSetBuilderVK::BindImage(u32 nameHash, const VkDescriptorImageInfo& imageInfo)
        {
            for (auto& bindInfo : _bindInfos)
            {
                if (nameHash == bindInfo.nameHash)
                {
                    BindImage(bindInfo.set, bindInfo.binding, imageInfo);
                    return;
                }
            }
        }

        void DescriptorSetBuilderVK::BindBuffer(i32 set, i32 binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType bufferType)
        {
            for (auto& bufferWrite : _bufferWrites) 
            {
                if (bufferWrite.dstBinding == binding && bufferWrite.dstSet == set)
                {
                    bufferWrite.bufferInfo = bufferInfo;
                    bufferWrite.descriptorType = bufferType;
                    return;
                }
            }

            BufferWriteDescriptor newWrite;
            newWrite.dstSet = set;
            newWrite.dstBinding = binding;
            newWrite.descriptorType = bufferType;
            newWrite.bufferInfo = bufferInfo;

            _bufferWrites.push_back(newWrite);
        }

        void DescriptorSetBuilderVK::BindBuffer(u32 nameHash, const VkDescriptorBufferInfo& bufferInfo)
        {
            for (auto& bindInfo : _bindInfos)
            {
                if (nameHash == bindInfo.nameHash)
                {
                    BindBuffer(bindInfo.set, bindInfo.binding, bufferInfo, bindInfo.descriptorType);
                    return;
                }
            }
        }

        void DescriptorSetBuilderVK::BindRayStructure(i32 set, i32 binding, const VkWriteDescriptorSetAccelerationStructureKHR& info)
        {
            for (auto& bufferWrite : _bufferWrites)
            {
                if (bufferWrite.dstBinding == binding && bufferWrite.dstSet == set)
                {
                    bufferWrite.accelinfo = info;
                    return;
                }
            }

            BufferWriteDescriptor newWrite;
            newWrite.dstSet = set;
            newWrite.dstBinding = binding;
            newWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            newWrite.accelinfo = info;

            _bufferWrites.push_back(newWrite);
        }

        void DescriptorSetBuilderVK::BindRayStructure(u32 nameHash, const VkWriteDescriptorSetAccelerationStructureKHR& info)
        {
            for (auto& bindInfo : _bindInfos)
            {
                if (nameHash == bindInfo.nameHash)
                {
                    BindRayStructure(bindInfo.set, bindInfo.binding, info);
                    return;
                }
            }
        }

        void DescriptorSetBuilderVK::UpdateDescriptor(i32 set, VkDescriptorSet& descriptor, RenderDeviceVK& device)
        {
            std::vector<VkWriteDescriptorSet> descriptorWrites;
            descriptorWrites.reserve(20);

            for (ImageWriteDescriptor& imageWrite : _imageWrites) 
            {
                if (imageWrite.dstSet == set) 
                {
                    VkWriteDescriptorSet newWrite = {};
                    newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    newWrite.pNext = nullptr;

                    newWrite.dstBinding = imageWrite.dstBinding;
                    newWrite.dstSet = descriptor;
                    newWrite.descriptorCount = 1;
                    newWrite.descriptorType = imageWrite.descriptorType;
                    newWrite.pImageInfo = &imageWrite.imageInfo;

                    //special case for the image arrays
                    if (imageWrite.imageArray != nullptr)
                    {
                        newWrite.descriptorCount = imageWrite.imageCount;
                        newWrite.pImageInfo = imageWrite.imageArray;
                    }

                    descriptorWrites.push_back(newWrite);
                }
            }

            for (BufferWriteDescriptor& bufferWrite : _bufferWrites) 
            {
                if (bufferWrite.dstSet == set) 
                {
                    VkWriteDescriptorSet newWrite = {};
                    newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    newWrite.pNext = nullptr;

                    newWrite.dstBinding = bufferWrite.dstBinding;
                    newWrite.dstSet = descriptor;
                    newWrite.descriptorCount = 1;
                    newWrite.descriptorType = bufferWrite.descriptorType;
                    newWrite.pBufferInfo = &bufferWrite.bufferInfo;

                    if (bufferWrite.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
                    {
                        newWrite.pBufferInfo = nullptr;
                        newWrite.pNext = &bufferWrite.accelinfo;
                    }

                    descriptorWrites.push_back(newWrite);
                }
            }

            vkUpdateDescriptorSets(device._device, static_cast<u32>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        VkDescriptorSet DescriptorSetBuilderVK::BuildDescriptor(i32 set, DescriptorLifetime lifetime)
        {
            VkDescriptorSetLayout* layout; 
            if (_pipelineType == PipelineType::Graphics)
            {
                layout = &_pipelineHandler->GetDescriptorSetLayout(_graphicsPipelineID, set);
            }
            else
            {
                layout = &_pipelineHandler->GetDescriptorSetLayout(_computePipelineID, set);
            }

            void* next = nullptr;
            u32 counts[1];
            counts[0] = 4096;

            VkDescriptorSetVariableDescriptorCountAllocateInfo setCounts = {};
            setCounts.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
            setCounts.pNext = nullptr;
            setCounts.descriptorSetCount = 1;
            setCounts.pDescriptorCounts = counts;

            for (const ImageWriteDescriptor& imageWrite : _imageWrites)
            {
                if (imageWrite.imageArray != nullptr && imageWrite.dstSet == set)
                {
                    counts[0] = imageWrite.imageCount;
                    next = &setCounts;
                }
            }

            VkDescriptorSet newSet = _parentPool->AllocateDescriptor(*layout, lifetime, next);
            UpdateDescriptor(set, newSet, *_parentPool->_device);
            return newSet;
        }

        VkDescriptorSet DescriptorMegaPoolVK::AllocateDescriptor(VkDescriptorSetLayout layout, DescriptorLifetime lifetime, void* next)
        {
            if (lifetime == DescriptorLifetime::Static)
            {
                VkDescriptorSet set;
                _staticHandle.Allocate(layout, set, next);
                return set;
            }
            else
            {
                VkDescriptorSet set;
                _dynamicHandle.Allocate(layout, set, next);
                return set;
            }
        }

        void DescriptorMegaPoolVK::Init(i32 numFrames, RenderDeviceVK* device)
        {
            _device = device;

            _dynamicAllocatorPool = DescriptorAllocatorPoolVK::Create(device, numFrames);
            _staticAllocatorPool = DescriptorAllocatorPoolVK::Create(device, 1);
            _dynamicHandle = _dynamicAllocatorPool->GetAllocator();
            _staticHandle = _staticAllocatorPool->GetAllocator();
        }

        void DescriptorMegaPoolVK::SetFrame(i32 frameNumber)
        {
            _dynamicAllocatorPool->Flip();
            _dynamicHandle = _dynamicAllocatorPool->GetAllocator();
        }
    }
}