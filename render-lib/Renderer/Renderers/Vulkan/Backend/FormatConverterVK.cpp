#include "FormatConverterVK.h"

#include <Utils/DebugHandler.h>

namespace Renderer
{
    namespace Backend
    {
        VkFormat FormatConverterVK::ToVkFormat(const ImageFormat format)
        {
            switch (format)
            {
                case ImageFormat::UNKNOWN:                  assert(false); break; // This is an invalid format
                case ImageFormat::R32G32B32A32_FLOAT:       return VK_FORMAT_R32G32B32A32_SFLOAT; // RGBA32, 128 bits per pixel
                case ImageFormat::R32G32B32A32_UINT:        return VK_FORMAT_R32G32B32A32_UINT;
                case ImageFormat::R32G32B32A32_SINT:        return VK_FORMAT_R32G32B32A32_SINT;
                case ImageFormat::R32G32B32_FLOAT:          return VK_FORMAT_R32G32B32_SFLOAT; // RGB32, 96 bits per pixel
                case ImageFormat::R32G32B32_UINT:           return VK_FORMAT_R32G32B32_UINT;
                case ImageFormat::R32G32B32_SINT:           return VK_FORMAT_R32G32B32_SINT;
                case ImageFormat::R16G16B16A16_FLOAT:       return VK_FORMAT_R16G16B16A16_SFLOAT; // RGBA16, 64 bits per pixel
                case ImageFormat::R16G16B16A16_UNORM:       return VK_FORMAT_R16G16B16A16_UNORM;
                case ImageFormat::R16G16B16A16_UINT:        return VK_FORMAT_R16G16B16A16_UINT;
                case ImageFormat::R16G16B16A16_SNORM:       return VK_FORMAT_R16G16B16A16_SNORM;
                case ImageFormat::R16G16B16A16_SINT:        return VK_FORMAT_R16G16B16A16_SINT;
                case ImageFormat::R32G32_FLOAT:             return VK_FORMAT_R32G32_SFLOAT; // RG32, 64 bits per pixel
                case ImageFormat::R32G32_UINT:              return VK_FORMAT_R32G32_UINT;
                case ImageFormat::R32G32_SINT:              return VK_FORMAT_R32G32_SINT;
                case ImageFormat::R10G10B10A2_UNORM:        return VK_FORMAT_A2R10G10B10_UNORM_PACK32; // RGB10A2, 32 bits per pixel
                case ImageFormat::R10G10B10A2_UINT:         return VK_FORMAT_A2R10G10B10_UINT_PACK32;
                case ImageFormat::R11G11B10_FLOAT:          return VK_FORMAT_B10G11R11_UFLOAT_PACK32; //RG11B10, 32 bits per pixel
                case ImageFormat::R8G8B8A8_UNORM:           return VK_FORMAT_R8G8B8A8_UNORM; //RGBA8, 32 bits per pixel
                case ImageFormat::R8G8B8A8_UNORM_SRGB:      return VK_FORMAT_R8G8B8A8_SRGB;
                case ImageFormat::R8G8B8A8_UINT:            return VK_FORMAT_R8G8B8A8_UINT;
                case ImageFormat::R8G8B8A8_SNORM:           return VK_FORMAT_R8G8B8A8_SNORM;
                case ImageFormat::R8G8B8A8_SINT:            return VK_FORMAT_R8G8B8A8_SINT;

                case ImageFormat::B8G8R8A8_UNORM:           return VK_FORMAT_B8G8R8A8_UNORM;
                case ImageFormat::B8G8R8A8_UNORM_SRGB:      return VK_FORMAT_B8G8R8A8_SRGB;
                case ImageFormat::B8G8R8A8_SNORM:           return VK_FORMAT_B8G8R8A8_SNORM;
                case ImageFormat::B8G8R8A8_UINT:            return VK_FORMAT_B8G8R8A8_UINT;
                case ImageFormat::B8G8R8A8_SINT:            return VK_FORMAT_B8G8R8A8_SINT;

                case ImageFormat::R16G16_FLOAT:             return VK_FORMAT_R16G16_SFLOAT; // RG16, 32 bits per pixel
                case ImageFormat::R16G16_UNORM:             return VK_FORMAT_R16G16_UNORM;
                case ImageFormat::R16G16_UINT:              return VK_FORMAT_R16G16_UINT;
                case ImageFormat::R16G16_SNORM:             return VK_FORMAT_R16G16_SNORM;
                case ImageFormat::R16G16_SINT:              return VK_FORMAT_R16G16_SINT;
                case ImageFormat::R32_FLOAT:                return VK_FORMAT_R32_SFLOAT; // R32, 32 bits per pixel
                case ImageFormat::R32_UINT:                 return VK_FORMAT_R32_UINT;
                case ImageFormat::R32_SINT:                 return VK_FORMAT_R32_SINT;
                case ImageFormat::R8G8_UNORM:               return VK_FORMAT_R8G8_UNORM; // RG8, 16 bits per pixel
                case ImageFormat::R8G8_UINT:                return VK_FORMAT_R8G8_UINT;
                case ImageFormat::R8G8_SNORM:               return VK_FORMAT_R8G8_SNORM;
                case ImageFormat::R8G8_SINT:                return VK_FORMAT_R8G8_SINT;
                case ImageFormat::R16_FLOAT:                return VK_FORMAT_R16_SFLOAT; // R16, 16 bits per pixel
                case ImageFormat::D16_UNORM:                return VK_FORMAT_D16_UNORM; // Depth instead of Red
                case ImageFormat::R16_UNORM:                return VK_FORMAT_R16_UNORM;
                case ImageFormat::R16_UINT:                 return VK_FORMAT_R16_UINT;
                case ImageFormat::R16_SNORM:                return VK_FORMAT_R16_SNORM;
                case ImageFormat::R16_SINT:                 return VK_FORMAT_R16_SINT;
                case ImageFormat::R8_UNORM:                 return VK_FORMAT_R8_UNORM; // R8, 8 bits per pixel
                case ImageFormat::R8_UINT:                  return VK_FORMAT_R8_UINT;
                case ImageFormat::R8_SNORM:                 return VK_FORMAT_R8_SNORM;
                case ImageFormat::R8_SINT:                  return VK_FORMAT_R8_SINT;
                default:
                    assert(false); // We have tried to convert a image format we don't know about, did we just add it?
            }
            return VK_FORMAT_UNDEFINED;
        }

        VkSampleCountFlagBits FormatConverterVK::ToVkSampleCount(const SampleCount sampleCount)
        {
            switch (sampleCount)
            {
                case SampleCount::SAMPLE_COUNT_1:    return VK_SAMPLE_COUNT_1_BIT;
                case SampleCount::SAMPLE_COUNT_2:    return VK_SAMPLE_COUNT_2_BIT;
                case SampleCount::SAMPLE_COUNT_4:    return VK_SAMPLE_COUNT_4_BIT;
                case SampleCount::SAMPLE_COUNT_8:    return VK_SAMPLE_COUNT_8_BIT;
                default:
                    assert(false); // We have tried to convert a image format we don't know about, did we just add it?
            }
            return VK_SAMPLE_COUNT_1_BIT;
        }

        VkFormat FormatConverterVK::ToVkFormat(const DepthImageFormat format)
        {
            switch (format)
            {
                case DepthImageFormat::UNKNOWN:                    assert(false); break; // This is an invalid format
                case DepthImageFormat::D32_FLOAT_S8X24_UINT:       return VK_FORMAT_D32_SFLOAT_S8_UINT;
                case DepthImageFormat::D32_FLOAT:                  return VK_FORMAT_D32_SFLOAT;
                case DepthImageFormat::R32_FLOAT:                  return VK_FORMAT_R32_SFLOAT;
                case DepthImageFormat::D24_UNORM_S8_UINT:          return VK_FORMAT_D24_UNORM_S8_UINT;
                case DepthImageFormat::D16_UNORM:                  return VK_FORMAT_D16_UNORM;
                case DepthImageFormat::R16_UNORM:                  return VK_FORMAT_R16_UNORM;

                default:
                    assert(false); // We have tried to convert a image format we don't know about, did we just add it?
            }
            return VK_FORMAT_UNDEFINED;
        }

        VkFormat FormatConverterVK::ToVkFormat(const InputFormat format)
        {
            switch (format)
            {
                // 4 bytes per component
                case InputFormat::R32G32B32A32_FLOAT:   return VK_FORMAT_R32G32B32A32_SFLOAT;
                case InputFormat::R32G32B32A32_UINT:    return VK_FORMAT_R32G32B32A32_UINT;
                case InputFormat::R32G32B32A32_SINT:    return VK_FORMAT_R32G32B32A32_SINT;
                case InputFormat::R32G32B32_FLOAT:      return VK_FORMAT_R32G32B32_SFLOAT;
                case InputFormat::R32G32B32_UINT:       return VK_FORMAT_R32G32B32_UINT;
                case InputFormat::R32G32B32_SINT:       return VK_FORMAT_R32G32B32_SINT;
                case InputFormat::R32G32_FLOAT:         return VK_FORMAT_R32G32_SFLOAT;
                case InputFormat::R32G32_UINT:          return VK_FORMAT_R32G32_UINT;
                case InputFormat::R32G32_SINT:          return VK_FORMAT_R32G32_SINT;
                case InputFormat::R32_FLOAT:            return VK_FORMAT_R32_SFLOAT;
                case InputFormat::R32_UINT:             return VK_FORMAT_R32_UINT;
                case InputFormat::R32_SINT:             return VK_FORMAT_R32_SINT;
                // 2 bytes per component
                case InputFormat::R16G16B16A16_FLOAT:   return VK_FORMAT_R16G16B16A16_SFLOAT;
                case InputFormat::R16G16B16A16_UINT:    return VK_FORMAT_R16G16B16A16_UINT;
                case InputFormat::R16G16B16A16_SINT:    return VK_FORMAT_R16G16B16A16_SINT;
                case InputFormat::R16G16_FLOAT:         return VK_FORMAT_R16G16_SFLOAT;
                case InputFormat::R16G16_UINT:          return VK_FORMAT_R16G16_UINT;
                case InputFormat::R16G16_SINT:          return VK_FORMAT_R16G16_SINT;
                case InputFormat::R16_FLOAT:            return VK_FORMAT_R16_SFLOAT;
                case InputFormat::R16_UINT:             return VK_FORMAT_R16_UINT;
                case InputFormat::R16_SINT:             return VK_FORMAT_R16_SINT;
                // 1 byte per component
                case InputFormat::R8G8B8A8_UNORM:       return VK_FORMAT_R8G8B8A8_UNORM;
                case InputFormat::R8G8B8A8_UINT:        return VK_FORMAT_R8G8B8A8_UINT;
                case InputFormat::R8G8B8A8_SINT:        return VK_FORMAT_R8G8B8A8_SINT;
                case InputFormat::R8G8_UINT:            return VK_FORMAT_R8G8_UINT;
                case InputFormat::R8G8_SINT:            return VK_FORMAT_R8G8_SINT;
                case InputFormat::R8_UINT:              return VK_FORMAT_R8_UINT;
                case InputFormat::R8_SINT:              return VK_FORMAT_R8_SINT;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more input formats?");
            }

            return VK_FORMAT_UNDEFINED;
        }

        VkPolygonMode FormatConverterVK::ToVkPolygonMode(const FillMode fillMode)
        {
            switch (fillMode)
            {
                case FillMode::SOLID:     return VK_POLYGON_MODE_FILL;
                case FillMode::WIREFRAME: return VK_POLYGON_MODE_LINE;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more fillmodes?");
            }

            return VK_POLYGON_MODE_FILL;
        }

        VkCullModeFlags FormatConverterVK::ToVkCullModeFlags(const CullMode cullMode)
        {
            switch (cullMode)
            {
                case CullMode::NONE: return VK_CULL_MODE_NONE;
                case CullMode::FRONT: return VK_CULL_MODE_FRONT_BIT;
                case CullMode::BACK: return VK_CULL_MODE_BACK_BIT;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more cullmodes?");
            }

            return VK_CULL_MODE_NONE;
        }

        VkFrontFace FormatConverterVK::ToVkFrontFace(const FrontFaceState frontFaceState)
        {
            switch (frontFaceState)
            {
                case FrontFaceState::CLOCKWISE:        return VK_FRONT_FACE_CLOCKWISE;
                case FrontFaceState::COUNTERCLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more frontface states?");
            }

            return VK_FRONT_FACE_CLOCKWISE;
        }

        VkBlendFactor FormatConverterVK::ToVkBlendFactor(const BlendMode blendMode)
        {
            switch (blendMode)
            {
                case BlendMode::ZERO:                return VK_BLEND_FACTOR_ZERO;
                case BlendMode::ONE:                 return VK_BLEND_FACTOR_ONE;
                case BlendMode::SRC_COLOR:           return VK_BLEND_FACTOR_SRC_COLOR;
                case BlendMode::INV_SRC_COLOR:       return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case BlendMode::SRC_ALPHA:           return VK_BLEND_FACTOR_SRC_ALPHA;
                case BlendMode::INV_SRC_ALPHA:       return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case BlendMode::DEST_ALPHA:          return VK_BLEND_FACTOR_DST_ALPHA;
                case BlendMode::INV_DEST_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case BlendMode::DEST_COLOR:          return VK_BLEND_FACTOR_DST_COLOR;
                case BlendMode::INV_DEST_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                case BlendMode::SRC_ALPHA_SAT:       return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
                case BlendMode::BLEND_FACTOR:        return VK_BLEND_FACTOR_CONSTANT_COLOR;
                case BlendMode::INV_BLEND_FACTOR:    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
                case BlendMode::SRC1_COLOR:          return VK_BLEND_FACTOR_SRC1_COLOR;
                case BlendMode::INV_SRC1_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
                case BlendMode::SRC1_ALPHA:          return VK_BLEND_FACTOR_SRC1_ALPHA;
                case BlendMode::INV_SRC1_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more blend modes?");
            }

            return VK_BLEND_FACTOR_ZERO;
        }

        VkBlendOp FormatConverterVK::ToVkBlendOp(const BlendOp blendOp)
        {
            switch (blendOp)
            {
                case BlendOp::ADD:             return VK_BLEND_OP_ADD;
                case BlendOp::SUBTRACT:        return VK_BLEND_OP_SUBTRACT;
                case BlendOp::REV_SUBTRACT:    return VK_BLEND_OP_REVERSE_SUBTRACT;
                case BlendOp::MIN:             return VK_BLEND_OP_MIN;
                case BlendOp::MAX:             return VK_BLEND_OP_MAX;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more blend ops?");
            }

            return VK_BLEND_OP_ADD;
        }

        VkColorComponentFlags FormatConverterVK::ToVkColorComponentFlags(const u8 componentFlags)
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

        VkLogicOp FormatConverterVK::ToVkLogicOp(const LogicOp logicOp)
        {
            switch (logicOp)
            {
                case LogicOp::CLEAR:           return VK_LOGIC_OP_CLEAR;
                case LogicOp::SET:             return VK_LOGIC_OP_SET;
                case LogicOp::COPY:            return VK_LOGIC_OP_COPY;
                case LogicOp::COPY_INVERTED:   return VK_LOGIC_OP_COPY_INVERTED;
                case LogicOp::NOOP:            return VK_LOGIC_OP_NO_OP;
                case LogicOp::INVERT:          return VK_LOGIC_OP_INVERT;
                case LogicOp::AND:             return VK_LOGIC_OP_AND;
                case LogicOp::NAND:            return VK_LOGIC_OP_NAND;
                case LogicOp::OR:              return VK_LOGIC_OP_OR;
                case LogicOp::NOR:             return VK_LOGIC_OP_NOR;
                case LogicOp::XOR:             return VK_LOGIC_OP_XOR;
                case LogicOp::EQUIV:           return VK_LOGIC_OP_EQUIVALENT;
                case LogicOp::AND_REVERSE:     return VK_LOGIC_OP_AND_REVERSE;
                case LogicOp::AND_INVERTED:    return VK_LOGIC_OP_AND_INVERTED;
                case LogicOp::OR_REVERSE:      return VK_LOGIC_OP_OR_REVERSE;
                case LogicOp::OR_INVERTED:     return VK_LOGIC_OP_OR_INVERTED;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more logic ops?");
            }

            return VK_LOGIC_OP_CLEAR;
        }

        VkCompareOp FormatConverterVK::ToVkCompareOp(const ComparisonFunc comparisonFunc)
        {
            switch (comparisonFunc)
            {
                case ComparisonFunc::NEVER:         return VK_COMPARE_OP_NEVER;
                case ComparisonFunc::LESS:          return VK_COMPARE_OP_LESS;
                case ComparisonFunc::EQUAL:         return VK_COMPARE_OP_EQUAL;
                case ComparisonFunc::LESS_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
                case ComparisonFunc::GREATER:       return VK_COMPARE_OP_GREATER;
                case ComparisonFunc::NOT_EQUAL:     return VK_COMPARE_OP_NOT_EQUAL;
                case ComparisonFunc::GREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case ComparisonFunc::ALWAYS:        return VK_COMPARE_OP_ALWAYS;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more comparison ops?");
            }

            return VK_COMPARE_OP_NEVER;
        }

        VkStencilOp FormatConverterVK::ToVkStencilOp(const StencilOp stencilOp)
        {
            switch (stencilOp)
            {
                case StencilOp::KEEP: return VK_STENCIL_OP_KEEP;
                case StencilOp::ZERO: return VK_STENCIL_OP_ZERO;
                case StencilOp::REPLACE: return VK_STENCIL_OP_REPLACE;
                case StencilOp::INCR_SAT: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
                case StencilOp::DECR_SAT: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
                case StencilOp::INVERT: return VK_STENCIL_OP_INVERT;
                case StencilOp::INCR: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
                case StencilOp::DECR: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more stencil ops?");
            }

            return VK_STENCIL_OP_KEEP;
        }

        VkFilter FormatConverterVK::ToVkFilterMag(SamplerFilter filter)
        {
            switch (filter)
            {
                case SamplerFilter::MIN_MAG_MIP_POINT:                           return VK_FILTER_NEAREST;
                case SamplerFilter::MIN_MAG_POINT_MIP_LINEAR:                    return VK_FILTER_NEAREST;
                case SamplerFilter::MIN_POINT_MAG_LINEAR_MIP_POINT:              return VK_FILTER_LINEAR;
                case SamplerFilter::MIN_POINT_MAG_MIP_LINEAR:                    return VK_FILTER_LINEAR;
                case SamplerFilter::MIN_LINEAR_MAG_MIP_POINT:                    return VK_FILTER_NEAREST;
                case SamplerFilter::MIN_LINEAR_MAG_POINT_MIP_LINEAR:             return VK_FILTER_NEAREST;
                case SamplerFilter::MIN_MAG_LINEAR_MIP_POINT:                    return VK_FILTER_LINEAR;
                case SamplerFilter::MIN_MAG_MIP_LINEAR:                          return VK_FILTER_LINEAR;
                case SamplerFilter::ANISOTROPIC:                                 return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_MIN_MAG_MIP_POINT:                return VK_FILTER_NEAREST;
                case SamplerFilter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:         return VK_FILTER_NEAREST;
                case SamplerFilter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:   return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:         return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:         return VK_FILTER_NEAREST;
                case SamplerFilter::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:  return VK_FILTER_NEAREST;
                case SamplerFilter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:         return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_MIN_MAG_MIP_LINEAR:               return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_ANISOTROPIC:                      return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_MIN_MAG_MIP_POINT:                   return VK_FILTER_NEAREST;
                case SamplerFilter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:      return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR:            return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:            return VK_FILTER_NEAREST;
                case SamplerFilter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:     return VK_FILTER_NEAREST;
                case SamplerFilter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR:                  return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_ANISOTROPIC:                         return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_MIN_MAG_MIP_POINT:                   return VK_FILTER_NEAREST;
                case SamplerFilter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:      return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:            return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:            return VK_FILTER_NEAREST;
                case SamplerFilter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:     return VK_FILTER_NEAREST;
                case SamplerFilter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_MIN_MAG_MIP_LINEAR:                  return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_ANISOTROPIC:                         return VK_FILTER_LINEAR;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more filters?");
            }

            return VK_FILTER_NEAREST;
        }

        VkFilter FormatConverterVK::ToVkFilterMin(SamplerFilter filter)
        {
            switch (filter)
            {
                case SamplerFilter::MIN_MAG_MIP_POINT:                           return VK_FILTER_NEAREST;
                case SamplerFilter::MIN_MAG_POINT_MIP_LINEAR:                    return VK_FILTER_NEAREST;
                case SamplerFilter::MIN_POINT_MAG_LINEAR_MIP_POINT:              return VK_FILTER_NEAREST;
                case SamplerFilter::MIN_POINT_MAG_MIP_LINEAR:                    return VK_FILTER_NEAREST;
                case SamplerFilter::MIN_LINEAR_MAG_MIP_POINT:                    return VK_FILTER_LINEAR;
                case SamplerFilter::MIN_LINEAR_MAG_POINT_MIP_LINEAR:             return VK_FILTER_LINEAR;
                case SamplerFilter::MIN_MAG_LINEAR_MIP_POINT:                    return VK_FILTER_LINEAR;
                case SamplerFilter::MIN_MAG_MIP_LINEAR:                          return VK_FILTER_LINEAR;
                case SamplerFilter::ANISOTROPIC:                                 return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_MIN_MAG_MIP_POINT:                return VK_FILTER_NEAREST;
                case SamplerFilter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:         return VK_FILTER_NEAREST;
                case SamplerFilter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:   return VK_FILTER_NEAREST;
                case SamplerFilter::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:         return VK_FILTER_NEAREST;
                case SamplerFilter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:         return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:  return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:         return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_MIN_MAG_MIP_LINEAR:               return VK_FILTER_LINEAR;
                case SamplerFilter::COMPARISON_ANISOTROPIC:                      return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_MIN_MAG_MIP_POINT:                   return VK_FILTER_NEAREST;
                case SamplerFilter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:      return VK_FILTER_NEAREST;
                case SamplerFilter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:     return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR:                  return VK_FILTER_LINEAR;
                case SamplerFilter::MINIMUM_ANISOTROPIC:                         return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_MIN_MAG_MIP_POINT:                   return VK_FILTER_NEAREST;
                case SamplerFilter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:      return VK_FILTER_NEAREST;
                case SamplerFilter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:            return VK_FILTER_NEAREST;
                case SamplerFilter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:     return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:            return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_MIN_MAG_MIP_LINEAR:                  return VK_FILTER_LINEAR;
                case SamplerFilter::MAXIMUM_ANISOTROPIC:                         return VK_FILTER_LINEAR;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more filters?");
            }

            return VK_FILTER_NEAREST;
        }

        VkSamplerAddressMode FormatConverterVK::ToVkSamplerAddressMode(TextureAddressMode mode)
        {
            switch (mode)
            {
                case TextureAddressMode::WRAP:         return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case TextureAddressMode::MIRROR:       return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                case TextureAddressMode::CLAMP:        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                case TextureAddressMode::BORDER:       return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                case TextureAddressMode::MIRROR_ONCE:  return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more modes?");
            }

            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }

        VkBorderColor FormatConverterVK::ToVkBorderColor(StaticBorderColor borderColor)
        {
            switch (borderColor)
            {
                case StaticBorderColor::TRANSPARENT_BLACK:  return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
                case StaticBorderColor::OPAQUE_BLACK:       return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
                case StaticBorderColor::OPAQUE_WHITE:       return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more colors?");
            }

            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        }

        VkIndexType FormatConverterVK::ToVkIndexType(IndexFormat indexFormat)
        {
            switch (indexFormat)
            {
                case IndexFormat::UInt16:   return VK_INDEX_TYPE_UINT16;
                case IndexFormat::UInt32:   return VK_INDEX_TYPE_UINT32;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more colors?");
            }
            return VK_INDEX_TYPE_UINT16;
        }

        VkPrimitiveTopology FormatConverterVK::ToVkPrimitiveTopology(PrimitiveTopology topology)
        {
            switch (topology)
            {
                case PrimitiveTopology::Lines:         return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                case PrimitiveTopology::LineStrip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                case PrimitiveTopology::Triangles:     return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more colors?");
            }
            return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        }

        bool FormatConverterVK::ToAnisotropyEnabled(SamplerFilter filter)
        {
            switch (filter)
            {
                case SamplerFilter::ANISOTROPIC:             return true;
                case SamplerFilter::COMPARISON_ANISOTROPIC:  return true;
                case SamplerFilter::MINIMUM_ANISOTROPIC:     return true;
                case SamplerFilter::MAXIMUM_ANISOTROPIC:     return true;
            }
            return false;
        }

        u32 FormatConverterVK::ToByteSize(const InputFormat format)
        {
            switch (format)
            {
                // 4 bytes per component
                case InputFormat::R32G32B32A32_FLOAT:   return 16;
                case InputFormat::R32G32B32A32_UINT:    return 16;
                case InputFormat::R32G32B32A32_SINT:    return 16;
                case InputFormat::R32G32B32_FLOAT:      return 12;
                case InputFormat::R32G32B32_UINT:       return 12;
                case InputFormat::R32G32B32_SINT:       return 12;
                case InputFormat::R32G32_FLOAT:         return 8;
                case InputFormat::R32G32_UINT:          return 8;
                case InputFormat::R32G32_SINT:          return 8;
                case InputFormat::R32_FLOAT:            return 4;
                case InputFormat::R32_UINT:             return 4;
                case InputFormat::R32_SINT:             return 4;
                // 2 bytes per component
                case InputFormat::R16G16B16A16_FLOAT:   return 8;
                case InputFormat::R16G16B16A16_UINT:    return 8;
                case InputFormat::R16G16B16A16_SINT:    return 8;
                case InputFormat::R16G16_FLOAT:         return 4;
                case InputFormat::R16G16_UINT:          return 4;
                case InputFormat::R16G16_SINT:          return 4;
                case InputFormat::R16_FLOAT:            return 2;
                case InputFormat::R16_UINT:             return 2;
                case InputFormat::R16_SINT:             return 2;
                // 1 byte per component
                case InputFormat::R8G8B8A8_UNORM:       return 4;
                case InputFormat::R8G8B8A8_UINT:        return 4;
                case InputFormat::R8G8B8A8_SINT:        return 4;
                case InputFormat::R8G8_UINT:            return 2;
                case InputFormat::R8G8_SINT:            return 2;
                case InputFormat::R8_UINT:              return 1;
                case InputFormat::R8_SINT:              return 1;
                default:
                    DebugHandler::PrintFatal("This should never hit, did we forget to update this function after adding more input formats?");
            }

            return 1;
        }
    }
}