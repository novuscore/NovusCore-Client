#pragma once
#include <NovusTypes.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "DescriptorAllocatorVK.h"
#include "ShaderHandlerVK.h"
#include "../../../Descriptors/GraphicsPipelineDesc.h"
#include "../../../Descriptors/ComputePipelineDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        class PipelineHandlerVK;
        class ShaderHandlerVK;

        enum class DescriptorLifetime 
        {
            Static,
            PerFrame
        };

        struct DescriptorMegaPoolVK;

        class DescriptorSetBuilderVK
        {
        public:
            DescriptorSetBuilderVK(GraphicsPipelineID pipelineID, PipelineHandlerVK* pipelineHandler, ShaderHandlerVK* shaderHandler, DescriptorMegaPoolVK* parentPool);
            DescriptorSetBuilderVK(ComputePipelineID pipelineID, PipelineHandlerVK* pipelineHandler, ShaderHandlerVK* shaderHandler, DescriptorMegaPoolVK* parentPool);

            void InitReflectData();

            void BindSampler(i32 set, i32 binding, VkDescriptorImageInfo& imageInfo);
            void BindSampler(u32 nameHash, VkDescriptorImageInfo& imageInfo);

            void BindImageArray(i32 set, i32 binding, VkDescriptorImageInfo* images, i32 count);
            void BindImageArray(u32 nameHash, VkDescriptorImageInfo* images, i32 count);

            void BindImage(i32 set, i32 binding, const VkDescriptorImageInfo& imageInfo, bool imageWrite = false);
            void BindImage(u32 nameHash, const VkDescriptorImageInfo& imageInfo);

            void BindStorageImage(i32 set, i32 binding, const VkDescriptorImageInfo& imageInfo);
            void BindStorageImage(u32 nameHash, const VkDescriptorImageInfo& imageInfo);

            void BindBuffer(i32 set, i32 binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType bufferType);
            void BindBuffer(u32 nameHash, const VkDescriptorBufferInfo& bufferInfo);

            void BindRayStructure(i32 set, i32 binding, const VkWriteDescriptorSetAccelerationStructureKHR& info);
            void BindRayStructure(u32 nameHash, const VkWriteDescriptorSetAccelerationStructureKHR& info);

            void UpdateDescriptor(i32 set, VkDescriptorSet& descriptor, RenderDeviceVK& device);
            VkDescriptorSet BuildDescriptor(i32 set, DescriptorLifetime lifetime);

        private:
            enum class PipelineType
            {
                Graphics,
                Compute,
            };

            struct ImageWriteDescriptor
            {
                int dstSet;
                int dstBinding;
                VkDescriptorType descriptorType;
                VkDescriptorImageInfo imageInfo;
                VkDescriptorImageInfo* imageArray{ nullptr };
                int imageCount;
            };

            struct BufferWriteDescriptor
            {
                int dstSet;
                int dstBinding;
                VkDescriptorType descriptorType;
                VkDescriptorBufferInfo bufferInfo;
                VkWriteDescriptorSetAccelerationStructureKHR accelinfo;
            };

        private:
            PipelineHandlerVK* _pipelineHandler;
            ShaderHandlerVK* _shaderHandler;
            DescriptorMegaPoolVK* _parentPool;

            PipelineType _pipelineType;

            GraphicsPipelineID _graphicsPipelineID;
            ComputePipelineID _computePipelineID;

            std::vector<Backend::BindInfo> _bindInfos;

            std::vector<ImageWriteDescriptor> _imageWrites;
            std::vector<BufferWriteDescriptor> _bufferWrites;
        };

        struct DescriptorAllocator
        {
            i32 maxDescriptors;
            i32 current_descriptors;
            VkDescriptorPool pool;
        };

        struct DescriptorMegaPoolVK
        {
            VkDescriptorSet AllocateDescriptor(VkDescriptorSetLayout layout, DescriptorLifetime lifetime, void* next = nullptr);

            void Init(i32 numFrames, RenderDeviceVK* device);
            void SetFrame(i32 frameNumber);

            DescriptorAllocatorHandleVK _dynamicHandle;
            DescriptorAllocatorHandleVK _staticHandle;

            DescriptorAllocatorPoolVK* _dynamicAllocatorPool;
            DescriptorAllocatorPoolVK* _staticAllocatorPool;

            RenderDeviceVK* _device;
        };
    }
}