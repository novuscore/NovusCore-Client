#include "SAO.h"

#include <Renderer/Renderer.h>
#include <Renderer/FrameResource.h>
#include <Renderer/CommandList.h>

namespace PostProcess
{
    struct SAOData : ISAOData
    {
        Renderer::ImageID linearizedDepthImage;
        Renderer::ImageID rawAOImage;
        Renderer::ImageID blurredImage;

        Renderer::DescriptorSet linearizeDepthDescriptorSet;
        Renderer::DescriptorSet downsampleDepthDescriptorSet;
        Renderer::DescriptorSet rawAODescriptorSet;
        Renderer::DescriptorSet blurDescriptorSet;

        Renderer::SamplerID sampler;
    };

    ISAOData* SAO::_data = new SAOData();

    constexpr u32 NUM_MIP_LEVELS = 5;

    void SAO::Init(Renderer::Renderer* renderer)
    {
        SAOData& data = *static_cast<SAOData*>(_data);

        Renderer::ImageDesc imageDesc;
        
        imageDesc.dimensions = vec2(1.0f, 1.0f);
        imageDesc.dimensionType = Renderer::ImageDimensionType::DIMENSION_SCALE;
        imageDesc.format = Renderer::ImageFormat::R8G8B8A8_UNORM;
        imageDesc.sampleCount = Renderer::SampleCount::SAMPLE_COUNT_1;
        imageDesc.debugName = "SAORawAO";
        data.rawAOImage = renderer->CreateImage(imageDesc);

        imageDesc.debugName = "SAOBlurred";
        data.blurredImage = renderer->CreateImage(imageDesc);

        imageDesc.format = Renderer::ImageFormat::R32_FLOAT;
        imageDesc.mipLevels = NUM_MIP_LEVELS;
        imageDesc.debugName = "SAOLinearDepth";
        data.linearizedDepthImage = renderer->CreateImage(imageDesc);

        Renderer::SamplerDesc samplerDesc;
        samplerDesc.enabled = true;
        samplerDesc.filter = Renderer::SamplerFilter::MIN_MAG_MIP_LINEAR;
        samplerDesc.addressU = Renderer::TextureAddressMode::WRAP;
        samplerDesc.addressV = Renderer::TextureAddressMode::WRAP;
        samplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
        samplerDesc.minLOD = 0.f;
        samplerDesc.maxLOD = 16.f;
        samplerDesc.shaderVisibility = Renderer::ShaderVisibility::PIXEL;
        
        data.sampler = renderer->CreateSampler(samplerDesc);
    }

    void SAO::CalculateSAO(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const Params& params)
    {
        SAOData& data = *static_cast<SAOData*>(_data);

        commandList.PushMarker("ScalableAmbientObscurance", Color::White);
        {
            LinearizeDepth(renderer, graphResources, commandList, frameIndex, params);

            ComputeRawAO(renderer, graphResources, commandList, frameIndex, params);

            // Horizontal blur
            {
                BlurParams blurParams;
                blurParams.input = data.rawAOImage;
                blurParams.output = data.blurredImage;
                blurParams.direction = uvec2(1, 0);

                commandList.PushMarker("Blur Horizontal", Color::White);
                Blur(renderer, graphResources, commandList, frameIndex, blurParams);
                commandList.PopMarker();
            }

            // Vertical blur
            {
                BlurParams blurParams;
                blurParams.input = data.blurredImage;
                blurParams.output = params.output;
                blurParams.direction = uvec2(0, 1);

                commandList.PushMarker("Blur Vertical", Color::White);
                Blur(renderer, graphResources, commandList, frameIndex, blurParams);
                commandList.PopMarker();
            }
        }
        commandList.PopMarker();
    }

    void SAO::LinearizeDepth(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const Params& params)
    {
        SAOData& data = *static_cast<SAOData*>(_data);

        commandList.PushMarker("LinearizeDepth", Color::White);
        commandList.ImageBarrier(params.depth);

        // Linearize the first mip
        {
            commandList.PushMarker("Linearize Mip 0", Color::White);

            // Setup pipeline
            Renderer::ComputeShaderDesc shaderDesc;
            shaderDesc.path = "PostProcess/SAO/LinearizeDepth.cs.hlsl";

            Renderer::ComputePipelineDesc pipelineDesc;
            graphResources.InitializePipelineDesc(pipelineDesc);

            pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);

            Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);
            commandList.BeginPipeline(pipeline);

            data.linearizeDepthDescriptorSet.Bind("_sampler", data.sampler);
            data.linearizeDepthDescriptorSet.Bind("_depth", params.depth);
            data.linearizeDepthDescriptorSet.BindStorage("_linearDepth", data.linearizedDepthImage);

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &data.linearizeDepthDescriptorSet, frameIndex);

            struct Constants
            {
                f32 nearPlane;
                f32 farPlane;
            };

            Constants* constants = graphResources.FrameNew<Constants>();
            constants->nearPlane = params.nearPlane;
            constants->farPlane = params.farPlane;

            commandList.PushConstant(constants, 0, sizeof(Constants));

            vec2 resolution = renderer->GetImageDimension(data.linearizedDepthImage, 0);
            uvec2 dispatchCount = ((resolution + vec2(31, 31)) / vec2(32, 32));
            commandList.Dispatch(dispatchCount.x, dispatchCount.y, 1);

            commandList.EndPipeline(pipeline);
            commandList.ImageBarrier(data.linearizedDepthImage);
            commandList.PopMarker();
        }

        // Downsample the other mips
        for (u32 i = 1; i < NUM_MIP_LEVELS; i++)
        {
            std::string markerName = "Downsample Mip " + std::to_string(i);
            commandList.PushMarker(markerName, Color::White);

            // Setup pipeline
            Renderer::ComputeShaderDesc shaderDesc;
            shaderDesc.path = "PostProcess/SAO/DownsampleDepth.cs.hlsl";

            Renderer::ComputePipelineDesc pipelineDesc;
            graphResources.InitializePipelineDesc(pipelineDesc);

            pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);

            Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);
            commandList.BeginPipeline(pipeline);

            data.downsampleDepthDescriptorSet.Bind("_source", data.linearizedDepthImage, i-1);
            data.downsampleDepthDescriptorSet.BindStorage("_destination", data.linearizedDepthImage, i);

            commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &data.downsampleDepthDescriptorSet, frameIndex);

            vec2 resolution = renderer->GetImageDimension(data.linearizedDepthImage, i);
            uvec2 dispatchCount = ((resolution + vec2(31, 31)) / vec2(32, 32));
            commandList.Dispatch(dispatchCount.x, dispatchCount.y, 1);

            commandList.EndPipeline(pipeline);
            commandList.ImageBarrier(data.linearizedDepthImage);
            commandList.PopMarker();
        }
        
        commandList.PopMarker();
    }

    void SAO::ComputeRawAO(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const Params& params)
    {
        SAOData& data = *static_cast<SAOData*>(_data);

        commandList.PushMarker("RawAO", Color::White);

        // Setup pipeline
        Renderer::ComputeShaderDesc shaderDesc;
        shaderDesc.path = "PostProcess/SAO/RawAO.cs.hlsl";

        Renderer::ComputePipelineDesc pipelineDesc;
        graphResources.InitializePipelineDesc(pipelineDesc);

        pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);

        Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);
        commandList.BeginPipeline(pipeline);

        data.rawAODescriptorSet.Bind("_depth", data.linearizedDepthImage);
        data.rawAODescriptorSet.BindStorage("_destination", data.rawAOImage);

        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &data.rawAODescriptorSet, frameIndex);

        struct Constants
        {
            f32 projScale; // The height in pixels of a 1m object if viewed from 1m away
            f32 radius; // World-space AO radius in scene units (r).  e.g., 1.0m
            f32 bias; // Bias to avoid AO in smooth corners, e.g., 0.01m
            f32 intensity; // Darkending factor, e.g., 1.0
            mat4x4 viewMatrix;
            mat4x4 invProjMatrix;
        };

        Constants* constants = graphResources.FrameNew<Constants>();
        constants->projScale = params.projScale;
        constants->radius = params.radius;
        constants->bias = params.bias;
        constants->intensity = params.intensity;
        constants->viewMatrix = params.viewMatrix;
        constants->invProjMatrix = params.invProjMatrix;

        commandList.PushConstant(constants, 0, sizeof(Constants));

        vec2 resolution = renderer->GetImageDimension(data.rawAOImage, 0);
        uvec2 dispatchCount = ((resolution + vec2(31, 31)) / vec2(32, 32));
        commandList.Dispatch(dispatchCount.x, dispatchCount.y, 1);

        commandList.EndPipeline(pipeline);
        commandList.ImageBarrier(data.rawAOImage);
        commandList.PopMarker();
    }

    void SAO::Blur(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const BlurParams& params)
    {
        SAOData& data = *static_cast<SAOData*>(_data);

        // Setup pipeline
        Renderer::ComputeShaderDesc shaderDesc;
        shaderDesc.path = "PostProcess/SAO/Blur.cs.hlsl";

        Renderer::ComputePipelineDesc pipelineDesc;
        graphResources.InitializePipelineDesc(pipelineDesc);

        pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);

        Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);
        commandList.BeginPipeline(pipeline);
        data.blurDescriptorSet.Bind("_source", params.input);
        data.blurDescriptorSet.BindStorage("_destination", params.output);

        commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &data.blurDescriptorSet, frameIndex);

        struct Constants
        {
            vec2 direction;
        };

        Constants* constants = graphResources.FrameNew<Constants>();
        constants->direction = params.direction;

        commandList.PushConstant(constants, 0, sizeof(Constants));

        vec2 resolution = renderer->GetImageDimension(params.output, 0);
        uvec2 dispatchCount = ((resolution + vec2(31, 31)) / vec2(32, 32));
        commandList.Dispatch(dispatchCount.x, dispatchCount.y, 1);

        commandList.EndPipeline(pipeline);
        commandList.ImageBarrier(params.output);
    }
}