#pragma once
#include <NovusTypes.h>
#include <functional>
#include <algorithm>
#include <Utils/StrongTypedef.h>
#include "../RenderStates.h"
#include "../RenderPassResources.h"

#include "VertexShaderDesc.h"
#include "PixelShaderDesc.h"
#include "ImageDesc.h"
#include "DepthImageDesc.h"
#include "RenderTargetDesc.h"

namespace Renderer
{
    class RenderGraph;

    struct GraphicsPipelineDesc
    {
        static const int MAX_INPUT_LAYOUTS = 8;

        GraphicsPipelineDesc()
        {
            std::fill_n(renderTargets, MAX_RENDER_TARGETS, RenderPassMutableResource::Invalid());
        }

        // This part of the descriptor is hashable in the PipelineHandler
        struct States
        {
            // States
            RasterizerState rasterizerState;
            DepthStencilState depthStencilState;
            BlendState blendState;

            InputLayout inputLayouts[MAX_INPUT_LAYOUTS];
            PrimitiveTopology primitiveTopology = PrimitiveTopology::Triangles;

            // Shaders
            VertexShaderID vertexShader = VertexShaderID::Invalid();
            PixelShaderID pixelShader = PixelShaderID::Invalid();
        };
        States states;

        // Everything below this isn't hashable in the PipelineHandler since it will depend on the RenderGraph (which gets recreated every frame)
        std::function<ImageID(RenderPassResource resource)> ResourceToImageID = nullptr;
        std::function<DepthImageID(RenderPassResource resource)> ResourceToDepthImageID = nullptr;
        std::function<ImageID(RenderPassMutableResource resource)> MutableResourceToImageID = nullptr;
        std::function<DepthImageID(RenderPassMutableResource resource)> MutableResourceToDepthImageID = nullptr;

        // Rendertargets
        RenderPassMutableResource renderTargets[MAX_RENDER_TARGETS];
        RenderPassMutableResource depthStencil = RenderPassMutableResource::Invalid();
    };

    // Lets strong-typedef an ID type with the underlying type of u16
    STRONG_TYPEDEF(GraphicsPipelineID, u16);
}