#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan_core.h>

#include "../../../RenderStates.h"

namespace Renderer
{
    namespace Backend
    {
        class FormatConverterVK
        {
        public:
            static VkFormat ToVkFormat(const ImageFormat format);
            static VkSampleCountFlagBits ToVkSampleCount(const SampleCount sampleCount);
            static VkFormat ToVkFormat(const DepthImageFormat format);
            static VkFormat ToVkFormat(const InputFormat format);
            static VkPolygonMode ToVkPolygonMode(const FillMode fillMode);
            static VkCullModeFlags ToVkCullModeFlags(const CullMode cullMode);
            static VkFrontFace ToVkFrontFace(const FrontFaceState frontFaceState);
            static VkBlendFactor ToVkBlendFactor(const BlendMode blendMode);
            static VkBlendOp ToVkBlendOp(const BlendOp blendOp);
            static VkColorComponentFlags ToVkColorComponentFlags(const u8 componentFlags);
            static VkLogicOp ToVkLogicOp(const LogicOp logicOp);
            static VkCompareOp ToVkCompareOp(const ComparisonFunc comparisonFunc);
            static VkStencilOp ToVkStencilOp(const StencilOp stencilOp);
            static VkFilter ToVkFilterMag(SamplerFilter filter);
            static VkFilter ToVkFilterMin(SamplerFilter filter);
            static VkSamplerAddressMode ToVkSamplerAddressMode(TextureAddressMode mode);
            static VkBorderColor ToVkBorderColor(StaticBorderColor borderColor);
            static VkIndexType ToVkIndexType(IndexFormat indexFormat);
            static VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology topology);

            static bool ToAnisotropyEnabled(SamplerFilter filter);
            static u32 ToByteSize(const InputFormat format);
        };
    }
}