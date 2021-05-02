#include <NovusTypes.h>

#include <Renderer/RenderGraphResources.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>

namespace Renderer
{
    class Renderer;
    class RenderGraphResources;
    class CommandList;
}

namespace PostProcess
{
    struct ISAOData {};

    class SAO
    {
    public:
        static void Init(Renderer::Renderer* renderer);

        struct Params
        {
            Renderer::DepthImageID depth;

            // LinearizeDepth settings
            f32 nearPlane;
            f32 farPlane;

            // AO settings
            f32 projScale; // The height in pixels of a 1m object if viewed from 1m away
            f32 radius; // World-space AO radius in scene units (r).  e.g., 1.0m
            f32 bias; // Bias to avoid AO in smooth corners, e.g., 0.01m
            f32 intensity; // Darkending factor, e.g., 1.0
            mat4x4 viewMatrix;
            mat4x4 invProjMatrix;

            Renderer::ImageID output;
        };

        static void CalculateSAO(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const Params& params);

    private:
        struct BlurParams
        {
            Renderer::ImageID input;
            Renderer::ImageID output;

            uvec2 direction;
        };

        static void LinearizeDepth(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const Params& params);
        static void ComputeRawAO(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const Params& params);
        static void Blur(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, u32 frameIndex, const BlurParams& params);

    private:
        static ISAOData* _data;
    };
}