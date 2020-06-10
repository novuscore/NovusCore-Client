#pragma once
#include <Renderer/InstanceData.h>
#include <Renderer/Descriptors/ModelDesc.h>

struct Model
{
    Renderer::ModelID modelId;
    Renderer::InstanceData instanceData;
};