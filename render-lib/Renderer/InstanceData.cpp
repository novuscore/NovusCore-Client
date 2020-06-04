#include "InstanceData.h"
#include "Renderer.h"

namespace Renderer
{
    void InstanceData::Init(Renderer* renderer)
    {
        _cb = renderer->CreateConstantBuffer<ModelCB>();
    }

    void InstanceData::Apply(u32 frameIndex)
    {
        assert(_cb != nullptr); // Check if we have initialized

        _cb->resource.colorMultiplier = colorMultiplier;
        _cb->resource.modelMatrix = modelMatrix;
        _cb->Apply(frameIndex);
    }

    void* InstanceData::GetGPUResource(u32 frameIndex)
    {
        assert(_cb != nullptr); // Check if we have initialized

        return _cb->GetGPUResource(frameIndex);
    }

}