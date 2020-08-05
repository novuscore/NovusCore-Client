#include "InstanceData.h"
#include "Renderer.h"

namespace Renderer
{
    void InstanceData::Init(Renderer* renderer)
    {
        //_cb = renderer->CreateConstantBuffer<ModelCB>();
    }

    void InstanceData::Apply(u32 frameIndex)
    {
        //assert(_cb != nullptr); // Check if we have initialized
        //
        //_cb->resource.colorMultiplier = colorMultiplier;
        //_cb->resource.modelMatrix = modelMatrix;
        //_cb->Apply(frameIndex);
    }

    void InstanceData::ApplyAll()
    {
        for (u32 i = 0; i < 2; i++)
        {
            Apply(i);
        }
    }

    void* InstanceData::GetDescriptor(u32 frameIndex)
    {
        //assert(_cb != nullptr); // Check if we have initialized
        //
        //return _cb->GetDescriptor(frameIndex);
    }

    void* InstanceData::GetBuffer(u32 frameIndex)
    {
        //assert(_cb != nullptr); // Check if we have initialized
        //
        //return _cb->GetBuffer(frameIndex);
    }

}