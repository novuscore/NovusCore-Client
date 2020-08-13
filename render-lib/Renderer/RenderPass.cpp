#include "RenderPass.h"
#include <cassert>
#include <algorithm>

namespace Renderer
{

    /*void RenderPass::SetPipeline(GraphicsPipelineID pipeline)
    {
        assert(_computePipeline == ComputePipelineID::Invalid()); // Only one type of pipeline is allowed to be set at a time
        _graphicsPipeline = pipeline;
    }

    void RenderPass::SetPipeline(ComputePipelineID pipeline)
    {
        assert(_graphicsPipeline == GraphicsPipelineID::Invalid()); // Only one type of pipeline is allowed to be set at a time
        _computePipeline = pipeline;
    }

    void RenderPass::AddRenderLayer(RenderLayer* renderLayer)
    {
        std::vector<RenderLayer*>::iterator it = std::find(_renderLayers.begin(), _renderLayers.end(), renderLayer);
        assert(it != _renderLayers.end()); // I don't think there is ever a reason to render the same layer twice with the same settings in the same pass...

        _renderLayers.push_back(renderLayer);
    }*/
}