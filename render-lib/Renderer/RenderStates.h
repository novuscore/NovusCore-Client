#pragma once
#include <NovusTypes.h>
#include <Utils/DebugHandler.h>

namespace Renderer
{
    static const int MAX_RENDER_TARGETS = 8;

    enum class FillMode
    {
        SOLID,
        WIREFRAME
    };

    enum class CullMode
    {
        NONE,
        FRONT,
        BACK
    };

    enum class SampleCount
    {
        SAMPLE_COUNT_1,
        SAMPLE_COUNT_2,
        SAMPLE_COUNT_4,
        SAMPLE_COUNT_8
    };

    constexpr int SampleCountToInt(SampleCount sampleCount)
    {
        switch (sampleCount)
        {
            case SampleCount::SAMPLE_COUNT_1: return 1;
            case SampleCount::SAMPLE_COUNT_2: return 2;
            case SampleCount::SAMPLE_COUNT_4: return 4;
            case SampleCount::SAMPLE_COUNT_8: return 8;
            default:
                DebugHandler::PrintFatal("Invalid sample count, did we just add to the enum?");
        }
        return 0;
    }

    enum class FrontFaceState
    {
        CLOCKWISE,
        COUNTERCLOCKWISE
    };

    struct RasterizerState
    {
        FillMode fillMode = FillMode::SOLID;
        CullMode cullMode = CullMode::BACK;
        FrontFaceState frontFaceMode = FrontFaceState::CLOCKWISE;
        bool depthBiasEnabled = false;
        i32 depthBias = 0;
        f32 depthBiasClamp = 0.0f;
        f32 depthBiasSlopeFactor = 0.0f;
        SampleCount sampleCount = SampleCount::SAMPLE_COUNT_1;
    };

    enum class PrimitiveTopology
    {
        Triangles,
        TriangleStrip,
        Lines,
        LineStrip,
    };

    enum class BlendMode
    {
        ZERO,
        ONE,
        SRC_COLOR,
        INV_SRC_COLOR,
        SRC_ALPHA,
        INV_SRC_ALPHA,
        DEST_ALPHA,
        INV_DEST_ALPHA,
        DEST_COLOR,
        INV_DEST_COLOR,
        SRC_ALPHA_SAT,
        BLEND_FACTOR,
        INV_BLEND_FACTOR,
        SRC1_COLOR,
        INV_SRC1_COLOR,
        SRC1_ALPHA,
        INV_SRC1_ALPHA
    };

    enum class BlendOp
    {
        ADD,
        SUBTRACT,
        REV_SUBTRACT,
        MIN,
        MAX
    };

    enum class LogicOp
    {
        CLEAR,
        SET,
        COPY,
        COPY_INVERTED,
        NOOP,
        INVERT,
        AND,
        NAND,
        OR,
        NOR,
        XOR,
        EQUIV,
        AND_REVERSE,
        AND_INVERTED,
        OR_REVERSE,
        OR_INVERTED
    };

    enum ColorWriteEnable
    {
        COLOR_WRITE_ENABLE_RED = 1,
        COLOR_WRITE_ENABLE_GREEN = 2,
        COLOR_WRITE_ENABLE_BLUE = 4,
        COLOR_WRITE_ENABLE_ALPHA = 8,
        COLOR_WRITE_ENABLE_ALL = (COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN | COLOR_WRITE_ENABLE_BLUE | COLOR_WRITE_ENABLE_ALPHA)
    };

    struct RTBlendState
    {
        bool blendEnable = false;
        bool logicOpEnable = false;

        BlendMode srcBlend = BlendMode::ONE;
        BlendMode destBlend = BlendMode::ZERO;
        BlendOp blendOp = BlendOp::ADD;

        BlendMode srcBlendAlpha = BlendMode::ONE;
        BlendMode destBlendAlpha = BlendMode::ZERO;
        BlendOp blendOpAlpha = BlendOp::ADD;

        LogicOp logicOp = LogicOp::NOOP;
        u8 renderTargetWriteMask = ColorWriteEnable::COLOR_WRITE_ENABLE_ALL;
    };

    struct BlendState
    {
        bool alphaToCoverageEnable = false;
        bool independentBlendEnable = false;
        RTBlendState renderTargets[MAX_RENDER_TARGETS];
    };

    enum class ComparisonFunc
    {
       NEVER,
       LESS,
       EQUAL,
       LESS_EQUAL,
       GREATER,
       NOT_EQUAL,
       GREATER_EQUAL,
       ALWAYS
    };

    enum class StencilOp
    {
        KEEP,
        ZERO,
        REPLACE,
        INCR_SAT,
        DECR_SAT,
        INVERT,
        INCR,
        DECR
    };

    struct DepthStencilOpDesc
    {
        StencilOp stencilFailOp = StencilOp::KEEP;
        StencilOp stencilDepthFailOp = StencilOp::KEEP;
        StencilOp stencilPassOp = StencilOp::KEEP;
        ComparisonFunc stencilFunc = ComparisonFunc::ALWAYS;
    };

    struct DepthStencilState
    {
        bool depthEnable = false;
        bool depthWriteEnable = false;
        ComparisonFunc depthFunc = ComparisonFunc::LESS;
        bool stencilEnable = false;
        u8 stencilReadMask = 255;
        u8 stencilWriteMask = 255;
        DepthStencilOpDesc frontFace;
        DepthStencilOpDesc backFace;
    };

    enum class ShaderVisibility
    {
        ALL,
        VERTEX,
        GEOMETRY,
        PIXEL
    };

    struct ConstantBufferState
    {
        bool enabled = false;
        ShaderVisibility shaderVisibility = ShaderVisibility::ALL;
    };

    enum class InputFormat
    {
        UNKNOWN,
        // 32 bit per component
        R32G32B32A32_FLOAT,
        R32G32B32A32_UINT,
        R32G32B32A32_SINT,
        R32G32B32_FLOAT,
        R32G32B32_UINT,
        R32G32B32_SINT,
        R32G32_FLOAT,
        R32G32_UINT,
        R32G32_SINT,
        R32_FLOAT,
        R32_UINT,
        R32_SINT,
        // 16 bit per component
        R16G16B16A16_FLOAT,
        R16G16B16A16_UINT,
        R16G16B16A16_SINT,
        R16G16_FLOAT,
        R16G16_UINT,
        R16G16_SINT,
        R16_FLOAT,
        R16_UINT,
        R16_SINT,
        // 8 bit per component
        R8G8B8A8_UNORM,
        R8G8B8A8_UINT,
        R8G8B8A8_SINT,
        R8G8_UINT,
        R8G8_SINT,
        R8_UINT,
        R8_SINT,

    };

    enum class InputClassification
    {
        PER_VERTEX,
        PER_INSTANCE
    };

    constexpr int INPUT_LAYOUT_NAME_MAX_LENGTH = 16;
    struct InputLayout
    {
        bool enabled = false;
        u32 index = 0;
        InputFormat format = InputFormat::UNKNOWN;
        u32 slot = 0;
        u32 alignedByteOffset = 0;
        InputClassification inputClassification = InputClassification::PER_VERTEX;
        u32 instanceDataStepRate = 0;

        const char* GetName() const
        {
            return _name;
        }
        void SetName(const std::string& name)
        {
            assert(name.length() < INPUT_LAYOUT_NAME_MAX_LENGTH); // The name you tried to set on the input layout is too long, either make it shorter or increase INPUT_LAYOUT_NAME_MAX_LENGTH
            memset(_name, 0, INPUT_LAYOUT_NAME_MAX_LENGTH);
            strcpy_s(_name, name.data());
        }

    private:
        char _name[INPUT_LAYOUT_NAME_MAX_LENGTH] = {};
    };

    enum class DepthClearFlags
    {
        DEPTH,
        STENCIL,
        BOTH
    };

    struct Viewport
    {
        f32 topLeftX = 0;
        f32 topLeftY = 0;
        f32 width = 0;
        f32 height = 0;
        f32 minDepth = 0;
        f32 maxDepth = 0;
    };

    struct ScissorRect
    {
        i32 left;
        i32 top;
        i32 right;
        i32 bottom;
    };

    enum class SamplerFilter
    {
        MIN_MAG_MIP_POINT,
        MIN_MAG_POINT_MIP_LINEAR,
        MIN_POINT_MAG_LINEAR_MIP_POINT,
        MIN_POINT_MAG_MIP_LINEAR,
        MIN_LINEAR_MAG_MIP_POINT,
        MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        MIN_MAG_LINEAR_MIP_POINT,
        MIN_MAG_MIP_LINEAR,
        ANISOTROPIC,
        COMPARISON_MIN_MAG_MIP_POINT,
        COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
        COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
        COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
        COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
        COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        COMPARISON_MIN_MAG_MIP_LINEAR,
        COMPARISON_ANISOTROPIC,
        MINIMUM_MIN_MAG_MIP_POINT,
        MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
        MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
        MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
        MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
        MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
        MINIMUM_MIN_MAG_MIP_LINEAR,
        MINIMUM_ANISOTROPIC,
        MAXIMUM_MIN_MAG_MIP_POINT,
        MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
        MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
        MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
        MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
        MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
        MAXIMUM_MIN_MAG_MIP_LINEAR,
        MAXIMUM_ANISOTROPIC
    };

    enum class TextureAddressMode
    {
        WRAP,
        MIRROR,
        CLAMP,
        BORDER,
        MIRROR_ONCE
    };

    enum class SamplerReductionMode
    {
        NONE,
        MAX,
        MIN
    };

    enum class StaticBorderColor
    {
        TRANSPARENT_BLACK,
        OPAQUE_BLACK,
        OPAQUE_WHITE
    };

    struct Sampler
    {
        bool enabled = false;
        SamplerFilter filter = SamplerFilter::MIN_MAG_MIP_POINT;
        TextureAddressMode addressU = TextureAddressMode::CLAMP;
        TextureAddressMode addressV = TextureAddressMode::CLAMP;
        TextureAddressMode addressW = TextureAddressMode::CLAMP;
        SamplerReductionMode mode = SamplerReductionMode::NONE;
        f32 mipLODBias = 0.0f;
        u32 maxAnisotropy = 0;
        ComparisonFunc comparisonFunc = ComparisonFunc::NEVER;
        StaticBorderColor borderColor = StaticBorderColor::OPAQUE_BLACK;
        f32 minLOD = 0.0f;
        f32 maxLOD = std::numeric_limits<f32>::max();
        ShaderVisibility shaderVisibility = ShaderVisibility::PIXEL;
        bool unnormalizedCoordinates = false;
    };

    enum class ImageFormat
    {
        UNKNOWN,
        R32G32B32A32_FLOAT,
        R32G32B32A32_UINT,
        R32G32B32A32_SINT,
        R32G32B32_FLOAT,
        R32G32B32_UINT,
        R32G32B32_SINT,
        R16G16B16A16_FLOAT,
        R16G16B16A16_UNORM,
        R16G16B16A16_UINT,
        R16G16B16A16_SNORM,
        R16G16B16A16_SINT,
        R32G32_FLOAT,
        R32G32_UINT,
        R32G32_SINT,
        R10G10B10A2_UNORM,
        R10G10B10A2_UINT,
        R11G11B10_FLOAT,
        R8G8B8A8_UNORM,
        R8G8B8A8_UNORM_SRGB,
        R8G8B8A8_UINT,
        R8G8B8A8_SNORM,
        R8G8B8A8_SINT,

        B8G8R8A8_UNORM,
        B8G8R8A8_UNORM_SRGB,
        B8G8R8A8_SNORM,
        B8G8R8A8_UINT,
        B8G8R8A8_SINT,

        R16G16_FLOAT,
        R16G16_UNORM,
        R16G16_UINT,
        R16G16_SNORM,
        R16G16_SINT,
        R32_FLOAT,
        R32_UINT,
        R32_SINT,
        R8G8_UNORM,
        R8G8_UINT,
        R8G8_SNORM,
        R8G8_SINT,
        R16_FLOAT,
        D16_UNORM,
        R16_UNORM,
        R16_UINT,
        R16_SNORM,
        R16_SINT,
        R8_UNORM,
        R8_UINT,
        R8_SNORM,
        R8_SINT
    };

    enum class IndexFormat
    {
        UInt16,
        UInt32,
    };

    enum class DepthImageFormat
    {
        UNKNOWN,

        // 32-bit Z w/ Stencil
        D32_FLOAT_S8X24_UINT,

        // No Stencil
        D32_FLOAT,
        R32_FLOAT,

        // 24-bit Z
        D24_UNORM_S8_UINT,

        // 16-bit Z w/o Stencil
        D16_UNORM,
        R16_UNORM
    };

    enum class ImageComponentType
    {
        FLOAT,
        UINT,
        SINT,
        UNORM,
        SNORM,

        COUNT
    };

    enum class ImageDimensionType
    {
        DIMENSION_ABSOLUTE, // vec2(1,1) means 1x1 pixels
        DIMENSION_SCALE,  // vec2(1,1) means 100% of window size
        DIMENSION_PYRAMID //vec2(1,1) means 100% of window size, does POW2 and mip levels
    };

    enum BufferUsage
    {
        INDIRECT_ARGUMENT_BUFFER   = (1 << 0),
        STORAGE_BUFFER             = (1 << 1),
        VERTEX_BUFFER              = (1 << 2),
        INDEX_BUFFER               = (1 << 3),
        UNIFORM_BUFFER             = (1 << 4),
        TRANSFER_SOURCE            = (1 << 5),
        TRANSFER_DESTINATION       = (1 << 6),
    };

    enum class BufferCPUAccess
    {
        None,
        WriteOnly,
        ReadOnly,
    };

    enum class PipelineBarrierType
    {
        TransferDestToIndirectArguments,
        TransferDestToComputeShaderRW,
        TransferDestToVertexBuffer,
        TransferDestToTransferSrc,
        ComputeWriteToIndirectArguments,
        ComputeWriteToVertexBuffer,
        ComputeWriteToVertexShaderRead,
        ComputeWriteToPixelShaderRead,
        ComputeWriteToComputeShaderRead,
        ComputeWriteToTransferSrc,
        AllCommands,
    };

    inline ImageComponentType ToImageComponentType(ImageFormat imageFormat)
    {
        switch (imageFormat)
        {
            case ImageFormat::R32G32B32A32_FLOAT:
            case ImageFormat::R32G32B32_FLOAT:
            case ImageFormat::R16G16B16A16_FLOAT:
            case ImageFormat::R32G32_FLOAT:
            case ImageFormat::R11G11B10_FLOAT:
            case ImageFormat::R16G16_FLOAT:
            case ImageFormat::R32_FLOAT:
            case ImageFormat::R16_FLOAT:
                return ImageComponentType::FLOAT;

            case ImageFormat::R32G32B32A32_UINT:
            case ImageFormat::R32G32B32_UINT:
            case ImageFormat::R16G16B16A16_UINT:
            case ImageFormat::R32G32_UINT:
            case ImageFormat::R10G10B10A2_UINT:
            case ImageFormat::R8G8B8A8_UINT:
            case ImageFormat::B8G8R8A8_UINT:
            case ImageFormat::R16G16_UINT:
            case ImageFormat::R32_UINT:
            case ImageFormat::R8G8_UINT:
            case ImageFormat::R16_UINT:
            case ImageFormat::R8_UINT:
                return ImageComponentType::UINT;

            case ImageFormat::R32G32B32A32_SINT:
            case ImageFormat::R32G32B32_SINT:
            case ImageFormat::R16G16B16A16_SINT:
            case ImageFormat::R32G32_SINT:
            case ImageFormat::R8G8B8A8_SINT:
            case ImageFormat::B8G8R8A8_SINT:
            case ImageFormat::R16G16_SINT:
            case ImageFormat::R32_SINT:
            case ImageFormat::R8G8_SINT:
            case ImageFormat::R16_SINT:
            case ImageFormat::R8_SINT:
                return ImageComponentType::SINT;
            
            case ImageFormat::R16G16B16A16_UNORM:
            case ImageFormat::R10G10B10A2_UNORM:
            case ImageFormat::R8G8B8A8_UNORM:
            case ImageFormat::R8G8B8A8_UNORM_SRGB:
            case ImageFormat::B8G8R8A8_UNORM:
            case ImageFormat::B8G8R8A8_UNORM_SRGB:
            case ImageFormat::R16G16_UNORM:
            case ImageFormat::R8G8_UNORM:
            case ImageFormat::D16_UNORM:
            case ImageFormat::R16_UNORM:
            case ImageFormat::R8_UNORM:
                return ImageComponentType::UNORM;

            case ImageFormat::R16G16B16A16_SNORM:
            case ImageFormat::R8G8B8A8_SNORM:
            case ImageFormat::B8G8R8A8_SNORM:
            case ImageFormat::R16G16_SNORM:
            case ImageFormat::R8G8_SNORM:
            case ImageFormat::R16_SNORM:
            case ImageFormat::R8_SNORM:
                return ImageComponentType::SNORM;

            case ImageFormat::UNKNOWN:
                DebugHandler::PrintFatal("This should never hit, we should catch unknowns earlier!");
                break;

            default:
                DebugHandler::PrintFatal("This should never hit, did we forget to add more cases after updating ImageFormat?");
                
        }
        return ImageComponentType::FLOAT;
    }
}