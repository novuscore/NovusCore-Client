#pragma once
#include <NovusTypes.h>
#include <Utils/DebugHandler.h>
#include <vulkan/vulkan.h>
#include "../../../RenderStates.h"

namespace Renderer
{
    namespace Backend
    {
        class FormatConverterVK
        {
        public:
            static inline VkFormat ToVkFormat(const ImageFormat format)
            {
                switch (format)
                {
                case IMAGE_FORMAT_UNKNOWN:                  assert(false); break; // This is an invalid format
                case IMAGE_FORMAT_R32G32B32A32_FLOAT:       return VK_FORMAT_R32G32B32A32_SFLOAT; // RGBA32, 128 bits per pixel
                case IMAGE_FORMAT_R32G32B32A32_UINT:        return VK_FORMAT_R32G32B32A32_UINT;
                case IMAGE_FORMAT_R32G32B32A32_SINT:        return VK_FORMAT_R32G32B32A32_SINT;
                case IMAGE_FORMAT_R32G32B32_FLOAT:          return VK_FORMAT_R32G32B32_SFLOAT; // RGB32, 96 bits per pixel
                case IMAGE_FORMAT_R32G32B32_UINT:           return VK_FORMAT_R32G32B32_UINT;
                case IMAGE_FORMAT_R32G32B32_SINT:           return VK_FORMAT_R32G32B32_SINT;
                case IMAGE_FORMAT_R16G16B16A16_FLOAT:       return VK_FORMAT_R16G16B16A16_SFLOAT; // RGBA16, 64 bits per pixel
                case IMAGE_FORMAT_R16G16B16A16_UNORM:       return VK_FORMAT_R16G16B16A16_UNORM;
                case IMAGE_FORMAT_R16G16B16A16_UINT:        return VK_FORMAT_R16G16B16A16_UINT;
                case IMAGE_FORMAT_R16G16B16A16_SNORM:       return VK_FORMAT_R16G16B16A16_SNORM;
                case IMAGE_FORMAT_R16G16B16A16_SINT:        return VK_FORMAT_R16G16B16A16_SINT;
                case IMAGE_FORMAT_R32G32_FLOAT:             return VK_FORMAT_R32G32_SFLOAT; // RG32, 64 bits per pixel
                case IMAGE_FORMAT_R32G32_UINT:              return VK_FORMAT_R32G32_UINT;
                case IMAGE_FORMAT_R32G32_SINT:              return VK_FORMAT_R32G32_SINT;
                case IMAGE_FORMAT_R10G10B10A2_UNORM:        return VK_FORMAT_A2R10G10B10_UNORM_PACK32; // RGB10A2, 32 bits per pixel
                case IMAGE_FORMAT_R10G10B10A2_UINT:         return VK_FORMAT_A2R10G10B10_UINT_PACK32;
                case IMAGE_FORMAT_R11G11B10_FLOAT:          return VK_FORMAT_B10G11R11_UFLOAT_PACK32; //RG11B10, 32 bits per pixel
                case IMAGE_FORMAT_R8G8B8A8_UNORM:           return VK_FORMAT_R8G8B8A8_UNORM; //RGBA8, 32 bits per pixel
                case IMAGE_FORMAT_R8G8B8A8_UNORM_SRGB:      return VK_FORMAT_R8G8B8A8_SRGB;
                case IMAGE_FORMAT_R8G8B8A8_UINT:            return VK_FORMAT_R8G8B8A8_UINT;
                case IMAGE_FORMAT_R8G8B8A8_SNORM:           return VK_FORMAT_R8G8B8A8_SNORM;
                case IMAGE_FORMAT_R8G8B8A8_SINT:            return VK_FORMAT_R8G8B8A8_SINT;

                case IMAGE_FORMAT_B8G8R8A8_UNORM:           return VK_FORMAT_B8G8R8A8_UNORM;
                case IMAGE_FORMAT_B8G8R8A8_UNORM_SRGB:      return VK_FORMAT_B8G8R8A8_SRGB;
                case IMAGE_FORMAT_B8G8R8A8_SNORM:           return VK_FORMAT_B8G8R8A8_SNORM;
                case IMAGE_FORMAT_B8G8R8A8_UINT:            return VK_FORMAT_B8G8R8A8_UINT;
                case IMAGE_FORMAT_B8G8R8A8_SINT:            return VK_FORMAT_B8G8R8A8_SINT;

                case IMAGE_FORMAT_R16G16_FLOAT:             return VK_FORMAT_R16G16_SFLOAT; // RG16, 32 bits per pixel
                case IMAGE_FORMAT_R16G16_UNORM:             return VK_FORMAT_R16G16_UNORM;
                case IMAGE_FORMAT_R16G16_UINT:              return VK_FORMAT_R16G16_UINT;
                case IMAGE_FORMAT_R16G16_SNORM:             return VK_FORMAT_R16G16_SNORM;
                case IMAGE_FORMAT_R16G16_SINT:              return VK_FORMAT_R16G16_SINT;
                case IMAGE_FORMAT_R32_FLOAT:                return VK_FORMAT_R32_SFLOAT; // R32, 32 bits per pixel
                case IMAGE_FORMAT_R32_UINT:                 return VK_FORMAT_R32_UINT;
                case IMAGE_FORMAT_R32_SINT:                 return VK_FORMAT_R32_SINT;
                case IMAGE_FORMAT_R8G8_UNORM:               return VK_FORMAT_R8G8_UNORM; // RG8, 16 bits per pixel
                case IMAGE_FORMAT_R8G8_UINT:                return VK_FORMAT_R8G8_UINT;
                case IMAGE_FORMAT_R8G8_SNORM:               return VK_FORMAT_R8G8_SNORM;
                case IMAGE_FORMAT_R8G8_SINT:                return VK_FORMAT_R8G8_SINT;
                case IMAGE_FORMAT_R16_FLOAT:                return VK_FORMAT_R16_SFLOAT; // R16, 16 bits per pixel
                case IMAGE_FORMAT_D16_UNORM:                return VK_FORMAT_D16_UNORM; // Depth instead of Red
                case IMAGE_FORMAT_R16_UNORM:                return VK_FORMAT_R16_UNORM;
                case IMAGE_FORMAT_R16_UINT:                 return VK_FORMAT_R16_UINT;
                case IMAGE_FORMAT_R16_SNORM:                return VK_FORMAT_R16_SNORM;
                case IMAGE_FORMAT_R16_SINT:                 return VK_FORMAT_R16_SINT;
                case IMAGE_FORMAT_R8_UNORM:                 return VK_FORMAT_R8_UNORM; // R8, 8 bits per pixel
                case IMAGE_FORMAT_R8_UINT:                  return VK_FORMAT_R8_UINT;
                case IMAGE_FORMAT_R8_SNORM:                 return VK_FORMAT_R8_SNORM;
                case IMAGE_FORMAT_R8_SINT:                  return VK_FORMAT_R8_SINT;
                default:
                    assert(false); // We have tried to convert a image format we don't know about, did we just add it?
                }
                return VK_FORMAT_UNDEFINED;
            }

            static inline VkSampleCountFlagBits ToVkSampleCount(const SampleCount sampleCount)
            {
                switch (sampleCount)
                {
                case SAMPLE_COUNT_1:    return VK_SAMPLE_COUNT_1_BIT;
                case SAMPLE_COUNT_2:    return VK_SAMPLE_COUNT_2_BIT;
                case SAMPLE_COUNT_4:    return VK_SAMPLE_COUNT_4_BIT;
                case SAMPLE_COUNT_8:    return VK_SAMPLE_COUNT_8_BIT;
                default:
                    assert(false); // We have tried to convert a image format we don't know about, did we just add it?
                }
                return VK_SAMPLE_COUNT_1_BIT;
            }

            static inline VkFormat ToVkFormat(const DepthImageFormat format)
            {
                switch (format)
                {
                case DEPTH_IMAGE_FORMAT_UNKNOWN:                    assert(false); break; // This is an invalid format
                case DEPTH_IMAGE_FORMAT_D32_FLOAT_S8X24_UINT:       return VK_FORMAT_D32_SFLOAT_S8_UINT;
                case DEPTH_IMAGE_FORMAT_D32_FLOAT:                  return VK_FORMAT_D32_SFLOAT;
                case DEPTH_IMAGE_FORMAT_R32_FLOAT:                  return VK_FORMAT_R32_SFLOAT;
                case DEPTH_IMAGE_FORMAT_D24_UNORM_S8_UINT:          return VK_FORMAT_D24_UNORM_S8_UINT;
                case DEPTH_IMAGE_FORMAT_D16_UNORM:                  return VK_FORMAT_D16_UNORM;
                case DEPTH_IMAGE_FORMAT_R16_UNORM:                  return VK_FORMAT_R16_UNORM;

                default:
                    assert(false); // We have tried to convert a image format we don't know about, did we just add it?
                }
                return VK_FORMAT_UNDEFINED;
            }

            static inline u32 ToByteSize(const InputFormat format)
            {
                switch (format)
                {
                    // 4 bytes per component
                case INPUT_FORMAT_R32G32B32A32_FLOAT:   return 16;
                case INPUT_FORMAT_R32G32B32A32_UINT:    return 16;
                case INPUT_FORMAT_R32G32B32A32_SINT:    return 16;
                case INPUT_FORMAT_R32G32B32_FLOAT:      return 12;
                case INPUT_FORMAT_R32G32B32_UINT:       return 12;
                case INPUT_FORMAT_R32G32B32_SINT:       return 12;
                case INPUT_FORMAT_R32G32_FLOAT:         return 8;
                case INPUT_FORMAT_R32G32_UINT:          return 8;
                case INPUT_FORMAT_R32G32_SINT:          return 8;
                case INPUT_FORMAT_R32_FLOAT:            return 4;
                case INPUT_FORMAT_R32_UINT:             return 4;
                case INPUT_FORMAT_R32_SINT:             return 4;
                    // 2 bytes per component
                case INPUT_FORMAT_R16G16B16A16_FLOAT:   return 8;
                case INPUT_FORMAT_R16G16B16A16_UINT:    return 8;
                case INPUT_FORMAT_R16G16B16A16_SINT:    return 8;
                case INPUT_FORMAT_R16G16_FLOAT:         return 4;
                case INPUT_FORMAT_R16G16_UINT:          return 4;
                case INPUT_FORMAT_R16G16_SINT:          return 4;
                case INPUT_FORMAT_R16_FLOAT:            return 2;
                case INPUT_FORMAT_R16_UINT:             return 2;
                case INPUT_FORMAT_R16_SINT:             return 2;
                    // 1 byte per component
                case INPUT_FORMAT_R8G8B8A8_UNORM:       return 4;
                case INPUT_FORMAT_R8G8B8A8_UINT:        return 4;
                case INPUT_FORMAT_R8G8B8A8_SINT:        return 4;
                case INPUT_FORMAT_R8G8_UINT:            return 2;
                case INPUT_FORMAT_R8G8_SINT:            return 2;
                case INPUT_FORMAT_R8_UINT:              return 1;
                case INPUT_FORMAT_R8_SINT:              return 1;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more input formats?");
                }

                return 1;
            }

            static inline VkFormat ToVkFormat(const InputFormat format)
            {
                switch (format)
                {
                    // 4 bytes per component
                case InputFormat::INPUT_FORMAT_R32G32B32A32_FLOAT:   return VK_FORMAT_R32G32B32A32_SFLOAT;
                case InputFormat::INPUT_FORMAT_R32G32B32A32_UINT:    return VK_FORMAT_R32G32B32A32_UINT;
                case InputFormat::INPUT_FORMAT_R32G32B32A32_SINT:    return VK_FORMAT_R32G32B32A32_SINT;
                case InputFormat::INPUT_FORMAT_R32G32B32_FLOAT:      return VK_FORMAT_R32G32B32_SFLOAT;
                case InputFormat::INPUT_FORMAT_R32G32B32_UINT:       return VK_FORMAT_R32G32B32_UINT;
                case InputFormat::INPUT_FORMAT_R32G32B32_SINT:       return VK_FORMAT_R32G32B32_SINT;
                case InputFormat::INPUT_FORMAT_R32G32_FLOAT:         return VK_FORMAT_R32G32_SFLOAT;
                case InputFormat::INPUT_FORMAT_R32G32_UINT:          return VK_FORMAT_R32G32_UINT;
                case InputFormat::INPUT_FORMAT_R32G32_SINT:          return VK_FORMAT_R32G32_SINT;
                case InputFormat::INPUT_FORMAT_R32_FLOAT:            return VK_FORMAT_R32_SFLOAT;
                case InputFormat::INPUT_FORMAT_R32_UINT:             return VK_FORMAT_R32_UINT;
                case InputFormat::INPUT_FORMAT_R32_SINT:             return VK_FORMAT_R32_SINT;
                    // 2 bytes per component
                case InputFormat::INPUT_FORMAT_R16G16B16A16_FLOAT:   return VK_FORMAT_R16G16B16A16_SFLOAT;
                case InputFormat::INPUT_FORMAT_R16G16B16A16_UINT:    return VK_FORMAT_R16G16B16A16_UINT;
                case InputFormat::INPUT_FORMAT_R16G16B16A16_SINT:    return VK_FORMAT_R16G16B16A16_SINT;
                case InputFormat::INPUT_FORMAT_R16G16_FLOAT:         return VK_FORMAT_R16G16_SFLOAT;
                case InputFormat::INPUT_FORMAT_R16G16_UINT:          return VK_FORMAT_R16G16_UINT;
                case InputFormat::INPUT_FORMAT_R16G16_SINT:          return VK_FORMAT_R16G16_SINT;
                case InputFormat::INPUT_FORMAT_R16_FLOAT:            return VK_FORMAT_R16_SFLOAT;
                case InputFormat::INPUT_FORMAT_R16_UINT:             return VK_FORMAT_R16_UINT;
                case InputFormat::INPUT_FORMAT_R16_SINT:             return VK_FORMAT_R16_SINT;
                    // 1 byte per component
                case InputFormat::INPUT_FORMAT_R8G8B8A8_UNORM:       return VK_FORMAT_R8G8B8A8_UNORM;
                case InputFormat::INPUT_FORMAT_R8G8B8A8_UINT:        return VK_FORMAT_R8G8B8A8_UINT;
                case InputFormat::INPUT_FORMAT_R8G8B8A8_SINT:        return VK_FORMAT_R8G8B8A8_SINT;
                case InputFormat::INPUT_FORMAT_R8G8_UINT:            return VK_FORMAT_R8G8_UINT;
                case InputFormat::INPUT_FORMAT_R8G8_SINT:            return VK_FORMAT_R8G8_SINT;
                case InputFormat::INPUT_FORMAT_R8_UINT:              return VK_FORMAT_R8_UINT;
                case InputFormat::INPUT_FORMAT_R8_SINT:              return VK_FORMAT_R8_SINT;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more input formats?");
                }

                return VK_FORMAT_UNDEFINED;
            }

            static inline VkPolygonMode ToVkPolygonMode(const FillMode fillMode)
            {
                switch (fillMode)
                {
                case FillMode::FILL_MODE_SOLID:     return VK_POLYGON_MODE_FILL;
                case FillMode::FILL_MODE_WIREFRAME: return VK_POLYGON_MODE_LINE;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more fillmodes?");
                }

                return VK_POLYGON_MODE_FILL;
            }

            static inline VkCullModeFlags ToVkCullModeFlags(const CullMode cullMode)
            {
                switch (cullMode)
                {
                case CullMode::CULL_MODE_NONE: return VK_CULL_MODE_NONE;
                case CullMode::CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
                case CullMode::CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more cullmodes?");
                }

                return VK_CULL_MODE_NONE;
            }

            static inline VkFrontFace ToVkFrontFace(const FrontFaceState frontFaceState)
            {
                switch (frontFaceState)
                {
                case FrontFaceState::FRONT_FACE_STATE_CLOCKWISE:        return VK_FRONT_FACE_CLOCKWISE;
                case FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more frontface states?");
                }

                return VK_FRONT_FACE_CLOCKWISE;
            }

            static inline VkBlendFactor ToVkBlendFactor(const BlendMode blendMode)
            {
                switch (blendMode)
                {
                case BlendMode::BLEND_MODE_ZERO:                return VK_BLEND_FACTOR_ZERO;
                case BlendMode::BLEND_MODE_ONE:                 return VK_BLEND_FACTOR_ONE;
                case BlendMode::BLEND_MODE_SRC_COLOR:           return VK_BLEND_FACTOR_SRC_COLOR;
                case BlendMode::BLEND_MODE_INV_SRC_COLOR:       return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case BlendMode::BLEND_MODE_SRC_ALPHA:           return VK_BLEND_FACTOR_SRC_ALPHA;
                case BlendMode::BLEND_MODE_INV_SRC_ALPHA:       return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case BlendMode::BLEND_MODE_DEST_ALPHA:          return VK_BLEND_FACTOR_DST_ALPHA;
                case BlendMode::BLEND_MODE_INV_DEST_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case BlendMode::BLEND_MODE_DEST_COLOR:          return VK_BLEND_FACTOR_DST_COLOR;
                case BlendMode::BLEND_MODE_INV_DEST_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                case BlendMode::BLEND_MODE_SRC_ALPHA_SAT:       return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
                case BlendMode::BLEND_MODE_BLEND_FACTOR:        return VK_BLEND_FACTOR_CONSTANT_COLOR;
                case BlendMode::BLEND_MODE_INV_BLEND_FACTOR:    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
                case BlendMode::BLEND_MODE_SRC1_COLOR:          return VK_BLEND_FACTOR_SRC1_COLOR;
                case BlendMode::BLEND_MODE_INV_SRC1_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
                case BlendMode::BLEND_MODE_SRC1_ALPHA:          return VK_BLEND_FACTOR_SRC1_ALPHA;
                case BlendMode::BLEND_MODE_INV_SRC1_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more blend modes?");
                }

                return VK_BLEND_FACTOR_ZERO;
            }

            static inline VkBlendOp ToVkBlendOp(const BlendOp blendOp)
            {
                switch (blendOp)
                {
                case BlendOp::BLEND_OP_ADD:             return VK_BLEND_OP_ADD;
                case BlendOp::BLEND_OP_SUBTRACT:        return VK_BLEND_OP_SUBTRACT;
                case BlendOp::BLEND_OP_REV_SUBTRACT:    return VK_BLEND_OP_REVERSE_SUBTRACT;
                case BlendOp::BLEND_OP_MIN:             return VK_BLEND_OP_MIN;
                case BlendOp::BLEND_OP_MAX:             return VK_BLEND_OP_MAX;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more blend ops?");
                }

                return VK_BLEND_OP_ADD;
            }

            static inline VkColorComponentFlags ToVkColorComponentFlags(const u8 componentFlags)
            {
                VkColorComponentFlags flags = 0;

                if (componentFlags & COLOR_WRITE_ENABLE_RED)
                    flags |= VK_COLOR_COMPONENT_R_BIT;
                if (componentFlags & COLOR_WRITE_ENABLE_GREEN)
                    flags |= VK_COLOR_COMPONENT_G_BIT;
                if (componentFlags & COLOR_WRITE_ENABLE_BLUE)
                    flags |= VK_COLOR_COMPONENT_B_BIT;
                if (componentFlags & COLOR_WRITE_ENABLE_ALPHA)
                    flags |= VK_COLOR_COMPONENT_A_BIT;

                return flags;
            }

            static inline VkLogicOp ToVkLogicOp(const LogicOp logicOp)
            {
                switch (logicOp)
                {
                case LogicOp::LOGIC_OP_CLEAR:           return VK_LOGIC_OP_CLEAR;
                case LogicOp::LOGIC_OP_SET:             return VK_LOGIC_OP_SET;
                case LogicOp::LOGIC_OP_COPY:            return VK_LOGIC_OP_COPY;
                case LogicOp::LOGIC_OP_COPY_INVERTED:   return VK_LOGIC_OP_COPY_INVERTED;
                case LogicOp::LOGIC_OP_NOOP:            return VK_LOGIC_OP_NO_OP;
                case LogicOp::LOGIC_OP_INVERT:          return VK_LOGIC_OP_INVERT;
                case LogicOp::LOGIC_OP_AND:             return VK_LOGIC_OP_AND;
                case LogicOp::LOGIC_OP_NAND:            return VK_LOGIC_OP_NAND;
                case LogicOp::LOGIC_OP_OR:              return VK_LOGIC_OP_OR;
                case LogicOp::LOGIC_OP_NOR:             return VK_LOGIC_OP_NOR;
                case LogicOp::LOGIC_OP_XOR:             return VK_LOGIC_OP_XOR;
                case LogicOp::LOGIC_OP_EQUIV:           return VK_LOGIC_OP_EQUIVALENT;
                case LogicOp::LOGIC_OP_AND_REVERSE:     return VK_LOGIC_OP_AND_REVERSE;
                case LogicOp::LOGIC_OP_AND_INVERTED:    return VK_LOGIC_OP_AND_INVERTED;
                case LogicOp::LOGIC_OP_OR_REVERSE:      return VK_LOGIC_OP_OR_REVERSE;
                case LogicOp::LOGIC_OP_OR_INVERTED:     return VK_LOGIC_OP_OR_INVERTED;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more logic ops?");
                }

                return VK_LOGIC_OP_CLEAR;
            }

            static inline VkCompareOp ToVkCompareOp(const ComparisonFunc comparisonFunc)
            {
                switch (comparisonFunc)
                {
                case ComparisonFunc::COMPARISON_FUNC_NEVER:         return VK_COMPARE_OP_NEVER;
                case ComparisonFunc::COMPARISON_FUNC_LESS:          return VK_COMPARE_OP_LESS;
                case ComparisonFunc::COMPARISON_FUNC_EQUAL:         return VK_COMPARE_OP_EQUAL;
                case ComparisonFunc::COMPARISON_FUNC_LESS_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
                case ComparisonFunc::COMPARISON_FUNC_GREATER:       return VK_COMPARE_OP_GREATER;
                case ComparisonFunc::COMPARISON_FUNC_NOT_EQUAL:     return VK_COMPARE_OP_NOT_EQUAL;
                case ComparisonFunc::COMPARISON_FUNC_GREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case ComparisonFunc::COMPARISON_FUNC_ALWAYS:        return VK_COMPARE_OP_ALWAYS;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more comparison ops?");
                }

                return VK_COMPARE_OP_NEVER;
            }

            static inline VkStencilOp ToVkStencilOp(const StencilOp stencilOp)
            {
                switch (stencilOp)
                {
                case StencilOp::STENCIL_OP_KEEP: return VK_STENCIL_OP_KEEP;
                case StencilOp::STENCIL_OP_ZERO: return VK_STENCIL_OP_ZERO;
                case StencilOp::STENCIL_OP_REPLACE: return VK_STENCIL_OP_REPLACE;
                case StencilOp::STENCIL_OP_INCR_SAT: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
                case StencilOp::STENCIL_OP_DECR_SAT: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
                case StencilOp::STENCIL_OP_INVERT: return VK_STENCIL_OP_INVERT;
                case StencilOp::STENCIL_OP_INCR: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
                case StencilOp::STENCIL_OP_DECR: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more stencil ops?");
                }

                return VK_STENCIL_OP_KEEP;
            }

            static inline VkFilter ToVkFilterMag(SamplerFilter filter)
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

            static inline VkFilter ToVkFilterMin(SamplerFilter filter)
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

            static inline bool ToAnisotropyEnabled(SamplerFilter filter)
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

            static inline VkSamplerAddressMode ToVkSamplerAddressMode(TextureAddressMode mode)
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

            static inline VkBorderColor ToVkBorderColor(StaticBorderColor borderColor)
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

            static inline VkIndexType ToVkIndexType(IndexFormat indexFormat)
            {
                switch (indexFormat)
                {
                case IndexFormat::UInt16:   return VK_INDEX_TYPE_UINT16;
                case IndexFormat::UInt32:   return VK_INDEX_TYPE_UINT32;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more colors?");
                }
                return VK_INDEX_TYPE_UINT16;
            }

            static inline VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology topology)
            {
                switch (topology)
                {
                case PrimitiveTopology::Lines:         return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                case PrimitiveTopology::LineStrip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                case PrimitiveTopology::Triangles:     return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                default:
                    NC_LOG_FATAL("This should never hit, did we forget to update this function after adding more colors?");
                }
                return (VkPrimitiveTopology)0;
            }
        };
    }
}